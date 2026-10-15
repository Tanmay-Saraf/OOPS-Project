#include "../src/SecurityLogger.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    const std::string filename = "security_test.log";

    {
        SecurityLogger logger(filename);
        logger.logEvent("CANARY_ALERT", "Canary record modification");
        logger.logEvent("REQUEST_BLOCKED", "Database is in lockdown");
    }

    std::ifstream file(filename);
    assert(file.is_open());

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    assert(content.find("CANARY_ALERT") != std::string::npos);
    assert(content.find("Canary record modification") != std::string::npos);
    assert(content.find("REQUEST_BLOCKED") != std::string::npos);
    assert(content.find("Database is in lockdown") != std::string::npos);

    std::cout << "All SecurityLogger tests passed!\n";
}
