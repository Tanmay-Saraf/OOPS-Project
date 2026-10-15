#ifndef CANARY_DETECTOR_H
#define CANARY_DETECTOR_H
#include "SecurityDetector.h"
#include <string>
#include <unordered_set>
class CanaryDetector : public SecurityDetector{
    private :
        std::unordered_set<std::string> canaryRecords;
    public :
        CanaryDetector();
        DetectionResult analyze(const oops::Request& request) override;     
};
#endif