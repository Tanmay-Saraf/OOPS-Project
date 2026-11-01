#include "security/EntropyDetector.h"
#include "TestCheck.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    EntropyDetector detector;

    oops::Request request{};
    request.command = oops::Command::Set;

    request.value = "aaaaaaaaaaaaaaaaaaaaaaaa";
    DetectionResult result = detector.analyze(request);
    TEST_CHECK(!result.Suspicious_flag);

    request.value = "hellohellohellohello";
    result = detector.analyze(request);
    TEST_CHECK(!result.Suspicious_flag);

    request.value = "aZ8!mQ2@xL5#pR9$wT4^nB7&cD6*eF0";
    result = detector.analyze(request);
    TEST_CHECK(result.Suspicious_flag);
    TEST_CHECK(result.HowSevere == Severity::MEDIUM);
    TEST_CHECK(result.Score == 60);

    request.command = oops::Command::Get;
    result = detector.analyze(request);
    TEST_CHECK(!result.Suspicious_flag);

    request.command = oops::Command::Set;
    request.value = "A9!x";
    result = detector.analyze(request);
    TEST_CHECK(!result.Suspicious_flag);

    EntropyDetector configurable(1.0, 4);
    request.value = "abcd";
    result = configurable.analyze(request);
    TEST_CHECK(result.Suspicious_flag);

    bool invalidThresholdRejected = false;

    try {
        EntropyDetector invalid(9.0, 16);
    } catch (const std::invalid_argument&) {
        invalidThresholdRejected = true;
    }

    TEST_CHECK(invalidThresholdRejected);

    std::cout << "All EntropyDetector tests passed!\n";
    return 0;
}