#ifndef MODIFICATION_ACTIVITY_DETECTION_H
#define MODIFICATION_ACTIVITY_DETECTION_H

#include "SecurityDetector.h"
#include <chrono>
#include <cstddef>
#include <deque>
#include <mutex>

class ModificationActivityDetector : public SecurityDetector {
private:
    using Clock = std::chrono::steady_clock;

    std::deque<Clock::time_point> modificationTimes;
    const std::size_t threshold;
    const std::chrono::milliseconds window;
    std::mutex detectorMutex;

public:
    explicit ModificationActivityDetector(
        std::size_t threshold = 100,
        std::chrono::milliseconds window = std::chrono::seconds(1)
    );

    DetectionResult analyze(const oops::Request& request) override;
};

#endif