#include "security/SecurityLogger.h"

#include "TestCheck.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

int main() {
    const auto uniqueSuffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()
    );
    const auto testDirectory = std::filesystem::temp_directory_path() /
        ("oops_security_logger_test_" + uniqueSuffix);
    std::filesystem::create_directories(testDirectory);
    const auto logPath = testDirectory / "security_test.log";
    const std::string filename = logPath.string();

    {
        SecurityLogger logger(filename);
        logger.logEvent("CANARY_ALERT", "Canary record modification");
        logger.logEvent("REQUEST_BLOCKED", "Database is in lockdown");

        std::vector<std::thread> workers;
        for (int i = 0; i < 4; ++i) {
            workers.emplace_back([&logger, i]() {
                for (int j = 0; j < 25; ++j) {
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
    TEST_CHECK(file.is_open());
    const std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    file.close();

    TEST_CHECK(content.find("CANARY_ALERT") != std::string::npos);
    TEST_CHECK(content.find("Canary record modification") != std::string::npos);
    TEST_CHECK(content.find("REQUEST_BLOCKED") != std::string::npos);
    TEST_CHECK(content.find("Database is in lockdown") != std::string::npos);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 25; ++j) {
            const std::string entry =
                "worker=" + std::to_string(i) +
                " item=" + std::to_string(j);
            TEST_CHECK(content.find(entry) != std::string::npos);
        }
    }

    const auto missingParent = testDirectory / "missing_parent";
    bool openFailureReported = false;
    try {
        SecurityLogger invalidLogger(
            (missingParent / "security.log").string()
        );
    } catch (const std::runtime_error&) {
        openFailureReported = true;
    }
    TEST_CHECK(openFailureReported);

    std::filesystem::remove_all(testDirectory);
    std::cout << "All SecurityLogger tests passed!\n";
    return 0;
}
