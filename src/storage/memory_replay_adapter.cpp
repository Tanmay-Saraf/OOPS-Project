#include "storage/memory_replay_adapter.hpp"

#include "memory/MemoryEngine.hpp"
#include "memory/Version.hpp"
#include "storage/wal_writer.hpp"

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>

namespace storage {
namespace {

std::optional<std::chrono::system_clock::time_point> convert_timestamp(std::uint64_t timestamp_us) {
    using Clock = std::chrono::system_clock;
    using Microseconds = std::chrono::microseconds;

    if(timestamp_us > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        return std::nullopt;
    }

    const auto max_clock_us = std::chrono::duration_cast<Microseconds>(Clock::duration::max()).count();
    if(max_clock_us < 0 || timestamp_us > static_cast<std::uint64_t>(max_clock_us)) {
        return std::nullopt;
    }

    const Microseconds timestamp(static_cast<std::int64_t>(timestamp_us));
    return Clock::time_point(std::chrono::duration_cast<Clock::duration>(timestamp));
}

std::optional<std::uint64_t> to_unix_microseconds(std::chrono::system_clock::time_point timestamp) {
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
        timestamp.time_since_epoch()
    ).count();

    if(micros < 0) {
        return std::nullopt;
    }

    return static_cast<std::uint64_t>(micros);
}

bool convert_record_type(RecordType type, oops::Operation& operation) {
    switch(type) {
        case RecordType::PUT:
            operation = oops::Operation::Put;
            return true;
        case RecordType::DELETE:
            operation = oops::Operation::Delete;
            return true;
        default:
            return false;
    }
}

bool convert_operation(oops::Operation operation, RecordType& type) {
    switch(operation) {
        case oops::Operation::Put:
            type = RecordType::PUT;
            return true;
        case oops::Operation::Delete:
            type = RecordType::DELETE;
            return true;
        default:
            return false;
    }
}

std::optional<oops::Version> make_version(const WalRecord& record) {
    oops::Operation operation = oops::Operation::Put;
    if(record.key.empty() || record.sequence_number == 0 || !convert_record_type(record.type, operation)) {
        return std::nullopt;
    }

    const auto timestamp = convert_timestamp(record.timestamp);
    if(!timestamp) {
        return std::nullopt;
    }

    const std::string value = operation == oops::Operation::Delete ? "" : record.value;
    return oops::Version(record.sequence_number, value, operation, *timestamp);
}

}

ReplayHandler make_memory_replay_handler(oops::MemoryStore& memory) {
    return [&memory](const WalRecord& record) {
        const auto version = make_version(record);
        return version.has_value() && memory.replayVersion(record.key, *version);
    };
}

ReplayHandler make_memory_replay_handler(oops::MemoryEngine& memory) {
    return [&memory](const WalRecord& record) {
        const auto version = make_version(record);
        return version.has_value() && memory.replayVersion(record.key, *version);
    };
}

ClearHandler make_memory_clear_handler(oops::MemoryEngine& memory) {
    return [&memory]() {
        memory.clear();
    };
}

oops::WalAppendHandler make_wal_append_handler(WalWriter& writer) {
    return [&writer](const std::string& key, const oops::Version& version) {
        if(version.id == 0 || key.empty()) {
            throw std::invalid_argument("Cannot persist a WAL record with an empty key or zero version ID");
        }

        RecordType type = RecordType::PUT;
        if(!convert_operation(version.operation, type)) {
            throw std::invalid_argument("Cannot persist an unknown Memory Engine operation");
        }

        const auto timestamp = to_unix_microseconds(version.timestamp);
        if(!timestamp) {
            throw std::invalid_argument("WAL timestamps must be non-negative Unix microseconds");
        }

        WalRecord record{
            type,
            version.id,
            *timestamp,
            key,
            version.operation == oops::Operation::Delete ? "" : version.value
        };

        writer.append(record);
    };
}

}
