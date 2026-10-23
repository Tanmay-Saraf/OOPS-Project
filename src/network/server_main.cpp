
#include "network/Server.hpp"
#ifdef OOPS_WITH_SECURITY
#include "integration/SecurityGuardAdapter.hpp"
#endif
#include <exception>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    try {
#ifdef OOPS_WITH_SECURITY
        oops::Server server(8080, std::make_unique<oops::SecurityGuardAdapter>());
#else
        oops::Server server(8080);
#endif

        std::cout << "Server listening on port 8080\n";
        std::thread server_thread([&server]() {
            server.run();
        });

        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto start = std::chrono::steady_clock::now();
        std::cerr << "Requesting shutdown NOW\n" << std::flush;

        server.requestShutdown();
        server_thread.join();

        auto end = std::chrono::steady_clock::now();
        std::cerr << "Shutdown completed in "
                << std::chrono::duration<double>(end - start).count()
                << " seconds\n";

        server_thread.join();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
