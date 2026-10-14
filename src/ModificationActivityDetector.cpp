// Implements ModificationActivityDetector::analyze() Overrides the analyze() interface inherited from SecurityDetector
// Counts SET and DELETE_RECORD requests in a sliding time window
#include "ModificationActivityDetector.h"
#include <algorithm>
#include <cstddef>
DetectionResult ModificationActivityDetector::analyze(const Request& request) {
    DetectionResult result;
    result.Detector = "ModificationActivity";
    result.Suspicious_flag = false;
    result.Score = 0;
    result.reason = "Normal activity";
    result.HowSevere = Severity::LOW;

    if (request.operation != Operation::SET &&
        request.operation != Operation::DELETE_RECORD) {
        return result;
    }

    auto now = std::chrono::steady_clock::now();
    modificationTimes.push_back(now);

    while (!modificationTimes.empty() &&
           std::chrono::duration_cast<std::chrono::milliseconds>(
               now - modificationTimes.front()).count() > 1000) {
        modificationTimes.pop_front();
    }

    if (modificationTimes.size() >= threshold) {
        result.Suspicious_flag = true;
        result.Score = 80;
        result.reason = "Modification rate exceeded threshold";
        result.HowSevere = Severity::HIGH;
    }

    return result;
}
