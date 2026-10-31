#include "security/EntropyDetector.h"
#include "TestCheck.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>
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

    request.value = "aZ8!mQ2@xL5#pR9$wT4^nB7&cD6*eF09";
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
    // Test: Reject NaN entropy threshold
    bool nanThresholdRejected = false;

    try {
        EntropyDetector invalidNan(
            std::numeric_limits<double>::quiet_NaN(), 16
        );
    } catch (const std::invalid_argument&) {
        nanThresholdRejected = true;
    }

    TEST_CHECK(nanThresholdRejected);
    // Calibration tests for text-oriented database workloads.
    // Candidate settings: entropy 4.7, minimum length 32 bytes.
    EntropyDetector candidateDetector(4.7, 32);

    oops::Request calibrationRequest{};
    calibrationRequest.command = oops::Command::Set;
    calibrationRequest.key = "calibration_key";

    // 1. Ordinary English text
    calibrationRequest.value =
        "The quick brown fox jumps over the lazy dog and then returns home.";

    DetectionResult calibrationResult =
        candidateDetector.analyze(calibrationRequest);

    TEST_CHECK(!calibrationResult.Suspicious_flag);

    // 2. JSON record
    calibrationRequest.value =
        R"({"user":"alice","status":"active","message":"hello world","count":42})";

    calibrationResult = candidateDetector.analyze(calibrationRequest);
    TEST_CHECK(!calibrationResult.Suspicious_flag);

    // 3. CSV-style data
    calibrationRequest.value =
        "alice,active,hello world,42,product description,customer note";

    calibrationResult = candidateDetector.analyze(calibrationRequest);
    TEST_CHECK(!calibrationResult.Suspicious_flag);

    // 4. Base64-encoded text
    calibrationRequest.value =
        "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZy4=";

    calibrationResult = candidateDetector.analyze(calibrationRequest);
    TEST_CHECK(calibrationResult.Suspicious_flag);

    // 5. Random-looking value: 32 bytes
    calibrationRequest.value =
        "aZ8!mQ2@xL5#pR9$wT4^nB7&cD6*eF09";

    calibrationResult = candidateDetector.analyze(calibrationRequest);
    TEST_CHECK(calibrationResult.Suspicious_flag);

    // 6. Random-looking value: 31 bytes.
    // Should not trigger because it is below the minimum length.
    calibrationRequest.value =
        "aZ8!mQ2@xL5#pR9$wT4^nB7&cD6*eF0";

    calibrationResult = candidateDetector.analyze(calibrationRequest);
    TEST_CHECK(!calibrationResult.Suspicious_flag);

    std::cout << "All EntropyDetector tests passed!\n";
    return 0;
}