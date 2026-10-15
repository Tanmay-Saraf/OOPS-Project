#ifndef SECURITY_DETECTOR_H
#define SECURITY_DETECTOR_H

#include "../include/common/Request.hpp"
#include "DetectionResult.h"

class SecurityDetector {
public:
    virtual DetectionResult analyze(const oops::Request& request) = 0;
    virtual ~SecurityDetector() = default;
};

#endif
