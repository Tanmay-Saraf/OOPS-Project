
#include "network/Server.hpp"
#include "memory/MemoryEngine.hpp"

#include "storage/memory_replay_adapter.hpp"
#include "storage/recovery_manager.hpp"
#include "storage/wal_writer.hpp"

#ifdef OOPS_WITH_SECURITY
#include "integration/SecurityGuardAdapter.hpp"
#endif

#include <atomic>
#include <charconv>
#include <chrono>
#include <cerrno>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <time.h>
#include <cstdint>

namespace {

int parsePort(const char* text) {
    const std::string value(text);
    int port = 0;
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto [ptr, error] = std::from_chars(begin, end, port);

    if (error != std::errc{} || ptr != end || port < 1 || port > 65535) {
        throw std::invalid_argument(
            "port must be an integer between 1 and 65535"
        );
    }

    return port;
}

} // namespace

int main(int argc, char* argv[]) {
    int port = 8080;

    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [port]\n";
        return 2;
    }

    try {
        if (argc == 2) {
            port = parsePort(argv[1]);
        }
    } catch (const std::exception& error) {
        std::cerr << "Invalid port: " << error.what() << '\n';
        return 2;
    }

    // Block shutdown signals before creating the server thread.
    sigset_t shutdown_signals;
    sigemptyset(&shutdown_signals);
    sigaddset(&shutdown_signals, SIGINT);
    sigaddset(&shutdown_signals, SIGTERM);

    const int mask_result =
        pthread_sigmask(SIG_BLOCK, &shutdown_signals, nullptr);

    if (mask_result != 0) {
        std::cerr << "Failed to block shutdown signals: "
                  << std::system_error(
                         mask_result,
                         std::generic_category()).what()
                  << '\n';
        return 1;
    }

    // Reset signal dispositions so sigtimedwait() can receive them.
    struct sigaction default_action {};
    default_action.sa_handler = SIG_DFL;
    sigemptyset(&default_action.sa_mask);

    if (sigaction(SIGINT, &default_action, nullptr) == -1 ||
        sigaction(SIGTERM, &default_action, nullptr) == -1) {
        std::cerr << "Failed to configure shutdown signals\n";
        return 1;
    }

    try {
        // ----------------------------------------------------
        // 1. Initialize persistent storage
        // ----------------------------------------------------

        const std::string wal_path = "data/kittydb.wal";

        std::filesystem::create_directories("data");

        // Writer must outlive MemoryEngine's WAL callback.
        storage::WalWriter wal_writer(wal_path);

        // ----------------------------------------------------
        // 2. Initialize Memory Engine
        // ----------------------------------------------------

        auto memory_engine = std::make_unique<oops::MemoryEngine>();

        // ----------------------------------------------------
        // 3. Recover memory from existing WAL
        // ----------------------------------------------------

        storage::RecoveryManager recovery_manager;

        const auto recovery_report = recovery_manager.recover(
            wal_path,
            storage::make_memory_replay_handler(*memory_engine)
        );

        if (!recovery_report.fully_recovered()) {
            std::string error_message = "WAL recovery failed";

            if (recovery_report.error_offset.has_value()) {
                error_message += " at byte offset " +
                    std::to_string(*recovery_report.error_offset);
            }

            if (recovery_report.replay_failed) {
                error_message += " (record replay failed)";
            }

            throw std::runtime_error(error_message);
        }

        std::cout << "WAL recovery completed. Restored "
                  << recovery_report.records_restored
                  << " records.\n";

        // ----------------------------------------------------
        // 4. Connect Memory Engine to WAL
        // ----------------------------------------------------

        memory_engine->setWalAppendHandler(
            storage::make_wal_append_handler(wal_writer)
        );

        // ----------------------------------------------------
        // 5. Create rollback handler
        // ----------------------------------------------------

        // Preserve a pointer before moving ownership into Server.
        auto* memory_engine_ptr = memory_engine.get();

        auto rollback_handler =
            [&, memory_engine_ptr](std::uint64_t timestamp) {

                const auto report = recovery_manager.rollback_to(
                    wal_writer,
                    storage::make_memory_clear_handler(*memory_engine_ptr),
                    storage::make_memory_replay_handler(*memory_engine_ptr),
                    timestamp
                );

                return report.fully_recovered();
            };
        
        auto recover_handler = [&, memory_engine_ptr]() {

        // Validate the WAL before clearing the current database.
        const auto validation_report = recovery_manager.recover(
            wal_path,
            [](const storage::WalRecord&) {
                return true;
            }
        );

        if (!validation_report.fully_recovered()) {
            return false;
        }

        // Clear current in-memory state.
        memory_engine_ptr->clear();

        // Replay the entire WAL.
        const auto report = recovery_manager.recover(
            wal_path,
            storage::make_memory_replay_handler(*memory_engine_ptr)
        );

        return report.fully_recovered();
    };

        // ----------------------------------------------------
        // 6. Initialize Server with Security + Memory
        // ----------------------------------------------------

#ifdef OOPS_WITH_SECURITY
        oops::Server server(
            port,
            std::make_unique<oops::SecurityGuardAdapter>(),
            std::move(memory_engine),
            rollback_handler,
            recover_handler
        );
#else
        oops::Server server(
            port,
            std::make_unique<oops::AllowAllGuard>(),
            std::move(memory_engine),
            rollback_handler,
            recover_handler
        );
#endif

        // ----------------------------------------------------
        // 7. Run Server
        // ----------------------------------------------------

        std::exception_ptr server_error;
        std::atomic<bool> server_finished{false};

        std::thread server_thread(
            [&server, &server_error, &server_finished]() {
                try {
                    server.run();
                } catch (...) {
                    server_error = std::current_exception();
                }

                server_finished.store(
                    true,
                    std::memory_order_release
                );
            }
        );

        std::cout << "Server listening on port " << port
                  << ". Press Ctrl+C to stop.\n"
                  << std::flush;

        bool received_shutdown_signal = false;
        std::exception_ptr wait_error;

        while (!server_finished.load(std::memory_order_acquire)) {
            timespec timeout{};
            timeout.tv_sec = 0;
            timeout.tv_nsec = 100'000'000;

            const int received =
                sigtimedwait(&shutdown_signals, nullptr, &timeout);

            if (received == SIGINT || received == SIGTERM) {
                received_shutdown_signal = true;
                break;
            }

            if (received == -1 && (errno == EAGAIN || errno == EINTR)) {
                continue;
            }

            if (received == -1) {
                wait_error = std::make_exception_ptr(
                    std::runtime_error(
                        "Failed while waiting for shutdown signal"
                    )
                );
                break;
            }
        }

        if ((received_shutdown_signal || wait_error) &&
            !server_finished.load(std::memory_order_acquire)) {

            std::cout << "\nReceived termination signal; shutting down...\n"
                      << std::flush;

            try {
                server.requestShutdown();
            } catch (...) {
                if (!wait_error) {
                    wait_error = std::current_exception();
                }
            }
        }

        server_thread.join();

        if (server_error) {
            std::rethrow_exception(server_error);
        }

        if (wait_error) {
            std::rethrow_exception(wait_error);
        }

        std::cout << "Server stopped cleanly.\n";

    } catch (const std::exception& error) {
        std::cerr << "Server error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
