
#include "network/Server.hpp"
#include <exception>
#include <iostream>

int main() {
    try {
        oops::Server server(8080);

        std::cout << "Server listening on port 8080\n";
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
