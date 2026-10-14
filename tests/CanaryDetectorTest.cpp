#include "../src/CanaryDetector.h"
#include <cassert>
#include <iostream>

int main() {
    CanaryDetector detector;

    Request request{};

    request.operation = Operation::GET;
    request.recordId = "canary_001";

    DetectionResult result = detector.analyze(request);
    assert(!result.Suspicious_flag);

    request.operation = Operation::SET;
    result = detector.analyze(request);
    assert(result.Suspicious_flag);
    assert(result.HowSevere == Severity::CRITICAL);

    request.operation = Operation::DELETE_RECORD;
    result = detector.analyze(request);
    assert(result.Suspicious_flag);

    request.operation = Operation::SET;
    request.recordId = "normal_record";
    result = detector.analyze(request);
    assert(!result.Suspicious_flag);

    std::cout << "All CanaryDetector tests passed!\n";
}
