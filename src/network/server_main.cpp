
#include "network/Server.hpp"
#ifdef OOPS_WITH_SECURITY
#include "integration/SecurityGuardAdapter.hpp"
#endif
#include <exception>
#include <iostream>

int main() {
    try {
#ifdef OOPS_WITH_SECURITY
        oops::Server server(8080, std::make_unique<oops::SecurityGuardAdapter>());
#else
        oops::Server server(8080);
#endif

        std::cout << "Server listening on port 8080\n";
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
