#ifndef SECURITY_DETECTOR_H
#define SECURITY_DETECTOR_H
#include "Request.h"
#include "DetectionResult.h"
class SecurityDetector{
    public:
        virtual DetectionResult analyze(const Request& request) = 0;

        virtual ~SecurityDetector() = default;

};
#endif