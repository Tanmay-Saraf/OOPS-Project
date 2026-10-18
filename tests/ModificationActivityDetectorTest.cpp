#include "security/ModificationActivityDetector.h"
#include "TestCheck.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
int main() {
    ModificationActivityDetector detector;

    oops::Request request{};
    request.command = oops::Command::Get;

    DetectionResult result = detector.analyze(request);
    TEST_CHECK(!result.Suspicious_flag);

    request.command = oops::Command::Set;

    for (int i = 0; i < 10; i++) {
        result = detector.analyze(request);
        TEST_CHECK(!result.Suspicious_flag);
    }

    for (int i = 0; i < 90; i++) {
        result = detector.analyze(request);
    }

    TEST_CHECK(result.Suspicious_flag);
    TEST_CHECK(result.HowSevere == Severity::HIGH);
        ModificationActivityDetector configurable(
        2, std::chrono::seconds(5)
    );

    result = configurable.analyze(request);
    TEST_CHECK(!result.Suspicious_flag);

    result = configurable.analyze(request);
    TEST_CHECK(result.Suspicious_flag);

    ModificationActivityDetector expiryDetector(
        2, std::chrono::milliseconds(20)
    );

    result = expiryDetector.analyze(request);
    TEST_CHECK(!result.Suspicious_flag);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    result = expiryDetector.analyze(request);
    TEST_CHECK(!result.Suspicious_flag);

    ModificationActivityDetector concurrentDetector(
        401, std::chrono::seconds(5)
    );

    std::vector<std::thread> workers;

    for (int i = 0; i < 4; i++) {
        workers.emplace_back([&]() {
            for (int j = 0; j < 100; j++) {
                DetectionResult concurrentResult =
                    concurrentDetector.analyze(request);

                TEST_CHECK(!concurrentResult.Suspicious_flag);
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    result = concurrentDetector.analyze(request);
    TEST_CHECK(result.Suspicious_flag);

    ModificationActivityDetector uniqueKeyDetector(
        1000,
        std::chrono::seconds(1),
        3,
        std::chrono::minutes(1)
    );
    oops::Request uniqueRequest{};
    uniqueRequest.command = oops::Command::Set;

    uniqueRequest.key = "unique_1";
    result = uniqueKeyDetector.analyze(uniqueRequest);
    TEST_CHECK(!result.Suspicious_flag);

    uniqueRequest.key = "unique_2";
    result = uniqueKeyDetector.analyze(uniqueRequest);
    TEST_CHECK(!result.Suspicious_flag);

    uniqueRequest.key = "unique_3";
    result = uniqueKeyDetector.analyze(uniqueRequest);
    TEST_CHECK(result.Suspicious_flag);
    TEST_CHECK(result.reason.find("Unique keys modified") != std::string::npos);

    ModificationActivityDetector uniqueExpiryDetector(
        100,
        std::chrono::seconds(1),
        2,
        std::chrono::milliseconds(20)
    );
    uniqueRequest.key = "expires_1";
    result = uniqueExpiryDetector.analyze(uniqueRequest);
    TEST_CHECK(!result.Suspicious_flag);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    uniqueRequest.key = "expires_2";
    result = uniqueExpiryDetector.analyze(uniqueRequest);
    TEST_CHECK(!result.Suspicious_flag);

    bool invalidUniqueThresholdRejected = false;
    try {
        ModificationActivityDetector invalidUniqueDetector(
            100, std::chrono::seconds(1), 0, std::chrono::minutes(1)
        );
    } catch (const std::invalid_argument&) {
        invalidUniqueThresholdRejected = true;
    }
    TEST_CHECK(invalidUniqueThresholdRejected);

    ModificationActivityDetector perMinuteDetector(
        1000,
        std::chrono::seconds(1),
        1000,
        std::chrono::minutes(1),
        3
    );
    oops::Request minuteRequest{};
    minuteRequest.command = oops::Command::Set;
    minuteRequest.key = "same_key";

    result = perMinuteDetector.analyze(minuteRequest);
    TEST_CHECK(!result.Suspicious_flag);
    result = perMinuteDetector.analyze(minuteRequest);
    TEST_CHECK(!result.Suspicious_flag);
    result = perMinuteDetector.analyze(minuteRequest);
    TEST_CHECK(result.Suspicious_flag);
    TEST_CHECK(result.reason.find("Per-minute") != std::string::npos);

    bool invalidMinuteThresholdRejected = false;
    try {
        ModificationActivityDetector invalidMinuteDetector(
            100, std::chrono::seconds(1), 100, std::chrono::minutes(1), 0
        );
    } catch (const std::invalid_argument&) {
        invalidMinuteThresholdRejected = true;
    }
    TEST_CHECK(invalidMinuteThresholdRejected);

    std::cout << "All detector tests passed!\n";

    return 0;
}
