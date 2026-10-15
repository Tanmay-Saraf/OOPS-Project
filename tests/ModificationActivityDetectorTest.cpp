#include "security/ModificationActivityDetector.h"
#include <cassert>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
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
        ModificationActivityDetector configurable(
        2, std::chrono::seconds(5)
    );

    result = configurable.analyze(request);
    assert(!result.Suspicious_flag);

    result = configurable.analyze(request);
    assert(result.Suspicious_flag);

    ModificationActivityDetector expiryDetector(
        2, std::chrono::milliseconds(20)
    );

    result = expiryDetector.analyze(request);
    assert(!result.Suspicious_flag);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    result = expiryDetector.analyze(request);
    assert(!result.Suspicious_flag);

    ModificationActivityDetector concurrentDetector(
        401, std::chrono::seconds(5)
    );

    std::vector<std::thread> workers;

    for (int i = 0; i < 4; i++) {
        workers.emplace_back([&]() {
            for (int j = 0; j < 100; j++) {
                DetectionResult concurrentResult =
                    concurrentDetector.analyze(request);

                assert(!concurrentResult.Suspicious_flag);
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    result = concurrentDetector.analyze(request);
    assert(result.Suspicious_flag);
    std::cout << "All detector tests passed!\n";

    return 0;
}
