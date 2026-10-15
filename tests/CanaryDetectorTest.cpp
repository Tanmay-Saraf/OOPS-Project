
#include "security/CanaryDetector.h"
#include <cassert>
#include <iostream>

int main() {
    CanaryDetector detector;
    oops::Request request{};

    request.command = oops::Command::Get;
    request.key = "canary_001";

    DetectionResult result = detector.analyze(request);
    assert(!result.Suspicious_flag);

    request.command = oops::Command::Set;
    result = detector.analyze(request);
    assert(result.Suspicious_flag);
    assert(result.HowSevere == Severity::CRITICAL);

    request.command = oops::Command::Delete;
    result = detector.analyze(request);
    assert(result.Suspicious_flag);

    request.command = oops::Command::Set;
    request.key = "normal_record";
    result = detector.analyze(request);
    assert(!result.Suspicious_flag);

    std::cout << "All CanaryDetector tests passed!\n";
}
