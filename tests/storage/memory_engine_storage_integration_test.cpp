#include "memory/MemoryEngine.hpp"
#include "storage/memory_replay_adapter.hpp"
#include "storage/recovery_manager.hpp"
#include "storage/wal_reader.hpp"
#include "storage/wal_writer.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <thread>
#include <string>

using namespace oops;

namespace {
Response set_value(MemoryEngine& engine, const std::string& key, const std::string& value) {
    Request request;
    request.command = Command::Set;
    request.key = key;
    request.value = value;
    return engine.execute(request);
}

Response delete_key(MemoryEngine& engine, const std::string& key) {
    Request request;
    request.command = Command::Delete;
    request.key = key;
    return engine.execute(request);
}

Response get_value(MemoryEngine& engine, const std::string& key) {
    Request request;
    request.command = Command::Get;
    request.key = key;
    return engine.execute(request);
}
}

int main() {
    const auto wal_path = std::filesystem::temp_directory_path() / "kittydb_memory_engine_storage_integration.wal";
    std::filesystem::remove(wal_path);

    {
        // Declare the writer before the engine so the engine and its callback
        // are destroyed before the writer.
        storage::WalWriter writer(wal_path.string());
        MemoryEngine live_engine;
        live_engine.setWalAppendHandler(storage::make_wal_append_handler(writer));

        // A normal SET is written to the WAL before it appears in MemoryEngine.
        Response response = set_value(live_engine, "colour", "blue");
        assert(response.status == ResponseStatus::Success);
        assert(get_value(live_engine, "colour").value == "blue");

        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        response = set_value(live_engine, "colour", "green");
        assert(response.status == ResponseStatus::Success);

        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        response = set_value(live_engine, "temporary", "value");
        assert(response.status == ResponseStatus::Success);

        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        response = delete_key(live_engine, "temporary");
        assert(response.status == ResponseStatus::Success);

        // Confirm that the exact operations and globally increasing IDs reached disk.
        storage::WalReader reader(wal_path.string());
        auto wal = reader.read_all();
        assert(wal.status == storage::WalReadStatus::OK);
        assert(wal.records.size() == 4);
        assert(wal.records[0].sequence_number == 1);
        assert(wal.records[0].type == storage::RecordType::PUT);
        assert(wal.records[0].key == "colour");
        assert(wal.records[0].value == "blue");
        assert(wal.records[1].sequence_number == 2);
        assert(wal.records[1].value == "green");
        assert(wal.records[3].sequence_number == 4);
        assert(wal.records[3].type == storage::RecordType::DELETE);

        // Startup recovery rebuilds a new MemoryEngine using its public API.
        MemoryEngine recovered_engine;
        storage::RecoveryManager recovery;
        auto report = recovery.recover(
            wal_path.string(),
            storage::make_memory_replay_handler(recovered_engine)
        );
        assert(report.fully_recovered());
        assert(report.records_restored == 4);
        assert(get_value(recovered_engine, "colour").status == ResponseStatus::Success);
        assert(get_value(recovered_engine, "colour").value == "green");
        assert(get_value(recovered_engine, "temporary").status == ResponseStatus::NotFound);

        // Permanent rollback is performed while the engine is quiescent
        // (no concurrent client writes), as required by this RecoveryManager API.
        report = recovery.rollback_to(
            writer,
            storage::make_memory_clear_handler(live_engine),
            storage::make_memory_replay_handler(live_engine),
            wal.records[0].timestamp
        );
        assert(report.fully_recovered());
        assert(report.records_restored == 1);
        assert(report.records_removed == 3);
        assert(get_value(live_engine, "colour").value == "blue");
        assert(get_value(live_engine, "temporary").status == ResponseStatus::NotFound);

        MemoryEngine restarted_engine;

        auto restart_report = recovery.recover(
            wal_path.string(),
            storage::make_memory_replay_handler(restarted_engine)
        );

        assert(restart_report.fully_recovered());

        assert(get_value(restarted_engine, "colour").value == "blue");

        assert(
            get_value(restarted_engine, "temporary").status
            == ResponseStatus::NotFound
        );

        // The writer remains usable after log replacement and replay advances the ID.
        response = set_value(live_engine, "after_rollback", "works");
        assert(response.status == ResponseStatus::Success);
        auto final_wal = reader.read_all();
        assert(final_wal.status == storage::WalReadStatus::OK);
        assert(final_wal.records.size() == 2);
        assert(final_wal.records[0].sequence_number == 1);
        assert(final_wal.records[1].sequence_number == 2);
        assert(final_wal.records[1].key == "after_rollback");
    }

    std::filesystem::remove(wal_path);
    return 0;
}
