#pragma once

#include "security/SecurityDetector.h"

#include <chrono>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

class ModificationActivityDetector : public SecurityDetector {
private:
    using Clock = std::chrono::steady_clock;
    using KeyEvent = std::pair<Clock::time_point, std::string>;

    std::deque<Clock::time_point> modificationTimes;
    std::deque<Clock::time_point> minuteModificationTimes;
    std::deque<KeyEvent> modifiedKeyEvents;
    std::unordered_map<std::string, std::size_t> modifiedKeyCounts;
    const std::size_t threshold;
    const std::chrono::milliseconds window;
    const std::size_t uniqueKeyThreshold;
    const std::chrono::milliseconds uniqueKeyWindow;
    const std::size_t modificationsPerMinuteThreshold;
    const std::chrono::milliseconds minuteWindow = std::chrono::minutes(1);
    std::mutex detectorMutex;

public:
    explicit ModificationActivityDetector(
        std::size_t threshold = 100,
        std::chrono::milliseconds window = std::chrono::seconds(1),
        std::size_t uniqueKeyThreshold = 100,
        std::chrono::milliseconds uniqueKeyWindow = std::chrono::minutes(1),
        std::size_t modificationsPerMinuteThreshold = 1000
    );

    DetectionResult analyze(const oops::Request& request) override;
};
