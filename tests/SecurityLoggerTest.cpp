#include "security/SecurityLogger.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <thread>
#include <vector>
int main() {
    const std::string filename = "security_test.log";

    {
        SecurityLogger logger(filename);

        logger.logEvent(
            "CANARY_ALERT",
            "Canary record modification"
        );

        logger.logEvent(
            "REQUEST_BLOCKED",
            "Database is in lockdown"
        );

        std::vector<std::thread> workers;

        for (int i = 0; i < 4; i++) {
            workers.emplace_back([&logger, i]() {
                for (int j = 0; j < 25; j++) {
                    logger.logEvent(
                        "CONCURRENT_TEST",
                        "worker=" + std::to_string(i) +
                        " item=" + std::to_string(j)
                    );
                }
            });
        }

        for (auto& worker : workers) {
            worker.join();
        }
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

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 25; j++) {
            const std::string entry =
                "worker=" + std::to_string(i) +
                " item=" + std::to_string(j);

            assert(content.find(entry) != std::string::npos);
        }
    }

    file.close();

    const auto missingParent =
        std::filesystem::temp_directory_path() /
        ("oops_logger_missing_" +
         std::to_string(
             std::chrono::steady_clock::now()
                 .time_since_epoch().count()
         ));

    std::filesystem::remove_all(missingParent);

    bool openFailureReported = false;

    try {
        SecurityLogger invalidLogger(
            (missingParent / "security.log").string()
        );
    } catch (const std::runtime_error&) {
        openFailureReported = true;
    }

    assert(openFailureReported);

    std::cout << "All SecurityLogger tests passed!\n";

    return 0;
}