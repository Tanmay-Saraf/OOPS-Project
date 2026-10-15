#include "security/ModificationActivityDetector.h"
#include <stdexcept>

ModificationActivityDetector::ModificationActivityDetector(
    std::size_t threshold,
    std::chrono::milliseconds window
) : threshold(threshold), window(window) {
    if (threshold == 0) {
        throw std::invalid_argument("Threshold must be greater than zero");
    }

    if (window <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument("Window must be positive");
    }
}

DetectionResult ModificationActivityDetector::analyze(
    const oops::Request& request
) {
    DetectionResult result;
    result.Detector = "ModificationActivity";
    result.Score = 0;
    result.Suspicious_flag = false;
    result.reason = "Normal activity";
    result.HowSevere = Severity::LOW;

    if (request.command != oops::Command::Set &&
        request.command != oops::Command::Delete) {
        return result;
    }

    std::lock_guard<std::mutex> lock(detectorMutex);

    const auto now = Clock::now();

    while (!modificationTimes.empty() &&
           now - modificationTimes.front() >= window) {
        modificationTimes.pop_front();
    }

    modificationTimes.push_back(now);

    if (modificationTimes.size() >= threshold) {
        result.Suspicious_flag = true;
        result.Score = 80;
        result.reason = "Modification rate exceeded threshold";
        result.HowSevere = Severity::HIGH;
    }

    return result;
}