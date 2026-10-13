
#include "../src/ModificationActivityDetector.h"
#include <cassert>
#include <iostream>

int main() {
    ModificationActivityDetector detector;

    Request request{};
    request.operation = Operation::GET;

    DetectionResult result = detector.analyze(request);
    assert(!result.Suspicious_flag);

    request.operation = Operation::SET;

    for (int i = 0; i < 10; i++) {
        result = detector.analyze(request);
        assert(!result.Suspicious_flag);
    }

    for (int i = 0; i < 90; i++) {
        result = detector.analyze(request);
    }

    assert(result.Suspicious_flag);
    assert(result.HowSevere == Severity::HIGH);

    std::cout << "All detector tests passed!\n";

    return 0;
}
