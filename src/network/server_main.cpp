#include "network/Server.hpp"

#ifdef OOPS_WITH_SECURITY
#include "integration/SecurityGuardAdapter.hpp"
#endif

#include <atomic>
#include <charconv>
#include <chrono>
#include <cerrno>
#include <exception>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <time.h>

namespace {

int parsePort(const char* text) {
    const std::string value(text);
    int port = 0;
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto [ptr, error] = std::from_chars(begin, end, port);

    if (error != std::errc{} || ptr != end || port < 1 || port > 65535) {
        throw std::invalid_argument("port must be an integer between 1 and 65535");
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

    // Block these signals before creating the server thread. New threads
    // inherit this mask, and the main thread receives them synchronously
    // through sigtimedwait(), avoiding work inside an asynchronous handler.
    sigset_t shutdown_signals;
    sigemptyset(&shutdown_signals);
    sigaddset(&shutdown_signals, SIGINT);
    sigaddset(&shutdown_signals, SIGTERM);

    const int mask_result = pthread_sigmask(SIG_BLOCK, &shutdown_signals, nullptr);
    if (mask_result != 0) {
        std::cerr << "Failed to block shutdown signals: "
                  << std::system_error(mask_result, std::generic_category()).what()
                  << '\n';
        return 1;
    }

    // Some shells mark SIGINT as ignored for background jobs. Reset the
    // dispositions after blocking so SIGINT/SIGTERM become pending signals
    // that sigtimedwait() can consume rather than being discarded.
    struct sigaction default_action {};
    default_action.sa_handler = SIG_DFL;
    sigemptyset(&default_action.sa_mask);
    if (sigaction(SIGINT, &default_action, nullptr) == -1 ||
        sigaction(SIGTERM, &default_action, nullptr) == -1) {
        std::cerr << "Failed to configure shutdown signals\n";
        return 1;
    }

    try {
#ifdef OOPS_WITH_SECURITY
        oops::Server server(port, std::make_unique<oops::SecurityGuardAdapter>());
#else
        oops::Server server(port);
#endif

        std::exception_ptr server_error;
        std::atomic<bool> server_finished{false};

        std::thread server_thread([&server, &server_error, &server_finished]() {
            try {
                server.run();
            } catch (...) {
                server_error = std::current_exception();
            }
            server_finished.store(true, std::memory_order_release);
        });

        std::cout << "Server listening on port " << port
                  << ". Press Ctrl+C to stop.\n" << std::flush;

        bool received_shutdown_signal = false;
        std::exception_ptr wait_error;

        while (!server_finished.load(std::memory_order_acquire)) {
            timespec timeout{};
            timeout.tv_sec = 0;
            timeout.tv_nsec = 100'000'000; // 100 ms

            const int received = sigtimedwait(&shutdown_signals, nullptr, &timeout);
            if (received == SIGINT || received == SIGTERM) {
                received_shutdown_signal = true;
                break;
            }

            if (received == -1 && (errno == EAGAIN || errno == EINTR)) {
                continue; // timeout or unrelated signal interruption
            }

            if (received == -1) {
                wait_error = std::make_exception_ptr(
                    std::runtime_error("Failed while waiting for shutdown signal"));
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
