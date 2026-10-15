#include "security/ModificationActivityDetector.h"
#include <cassert>
#include <iostream>

int main() {
    ModificationActivityDetector detector;

    oops::Request request{};
    request.command = oops::Command::Get;

    DetectionResult result = detector.analyze(request);
    assert(!result.Suspicious_flag);

    request.command = oops::Command::Set;

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
