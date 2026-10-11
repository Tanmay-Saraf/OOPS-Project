#ifndef MODIFICATION_ACTIVITY_DETECTION_H
#define MODIFICATION_ACTIVITY_DETECTION_H
#include "SecurityDetector.h"
#include <deque>
#include <chrono>
#include <string>
class ModificationActivityDetector : public SecurityDetector{
    private : 
        std::deque<std::chrono::steady_clock::time_point> modificationTimes;
        size_t threshold = 100;
    public :
        DetectionResult analyze(const Request& request)override;
};
#endif