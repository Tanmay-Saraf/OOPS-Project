#include "security/ModificationActivityDetector.h"

#include <stdexcept>

ModificationActivityDetector::ModificationActivityDetector(
    std::size_t threshold,
    std::chrono::milliseconds window,
    std::size_t uniqueKeyThreshold,
    std::chrono::milliseconds uniqueKeyWindow,
    std::size_t modificationsPerMinuteThreshold
) : threshold(threshold),
    window(window),
    uniqueKeyThreshold(uniqueKeyThreshold),
    uniqueKeyWindow(uniqueKeyWindow),
    modificationsPerMinuteThreshold(modificationsPerMinuteThreshold) {
    if (threshold == 0) {
        throw std::invalid_argument("Threshold must be greater than zero");
    }

    if (window <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument("Window must be positive");
    }

    if (uniqueKeyThreshold == 0) {
        throw std::invalid_argument(
            "Unique-key threshold must be greater than zero"
        );
    }

    if (modificationsPerMinuteThreshold == 0) {
        throw std::invalid_argument(
            "Per-minute threshold must be greater than zero"
        );
    }

    if (uniqueKeyWindow <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument("Unique-key window must be positive");
    }
}

DetectionResult ModificationActivityDetector::analyze(
    const oops::Request& request
) {
    DetectionResult result{};
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

    while (!minuteModificationTimes.empty() &&
           now - minuteModificationTimes.front() >= minuteWindow) {
        minuteModificationTimes.pop_front();
    }

    while (!modifiedKeyEvents.empty() &&
           now - modifiedKeyEvents.front().first >= uniqueKeyWindow) {
        const std::string& expiredKey = modifiedKeyEvents.front().second;
        auto countIt = modifiedKeyCounts.find(expiredKey);
        if (countIt != modifiedKeyCounts.end()) {
            if (--countIt->second == 0) {
                modifiedKeyCounts.erase(countIt);
            }
        }
        modifiedKeyEvents.pop_front();
    }

    modificationTimes.push_back(now);
    minuteModificationTimes.push_back(now);

    if (!request.key.empty()) {
        modifiedKeyEvents.emplace_back(now, request.key);
        ++modifiedKeyCounts[request.key];
    }

    const bool highModificationRate = modificationTimes.size() >= threshold;
    const bool highMinuteRate =
        minuteModificationTimes.size() >= modificationsPerMinuteThreshold;
    const bool highUniqueKeyRate = modifiedKeyCounts.size() >= uniqueKeyThreshold;

    if (highModificationRate || highMinuteRate || highUniqueKeyRate) {
        result.Suspicious_flag = true;
        result.Score = 80;
        result.HowSevere = Severity::HIGH;

        if (highModificationRate && highMinuteRate && highUniqueKeyRate) {
            result.reason =
                "Per-second rate, per-minute rate, and unique-key thresholds exceeded";
        } else if (highModificationRate && highMinuteRate) {
            result.reason =
                "Per-second and per-minute modification-rate thresholds exceeded";
        } else if (highModificationRate && highUniqueKeyRate) {
            result.reason =
                "Modification rate and unique-key rate exceeded thresholds";
        } else if (highMinuteRate && highUniqueKeyRate) {
            result.reason =
                "Per-minute modification rate and unique-key thresholds exceeded";
        } else if (highUniqueKeyRate) {
            result.reason =
                "Unique keys modified within the configured window exceeded threshold";
        } else if (highMinuteRate) {
            result.reason = "Per-minute modification rate exceeded threshold";
        } else {
            result.reason = "Modification rate exceeded threshold";
        }
    }

    return result;
}
