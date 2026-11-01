#pragma once

#include "common/Request.hpp"
#include "security/DetectionResult.h"

class SecurityDetector {
public:
    virtual DetectionResult analyze(const oops::Request& request) = 0;
    virtual ~SecurityDetector() = default;
};
