
#ifndef SECURITY_ENGINE_H
#define SECURITY_ENGINE_H

#include "common/Request.hpp"
#include "DetectionResult.h"
#include "ModificationActivityDetector.h"
#include "CanaryDetector.h"
#include "StateManager.h"
#include "SecurityLogger.h"
#include <string>

struct SecurityDecision {z`
    bool allowed;
    std::string reason;
    SecurityState currentState;
};

class SecurityEngine {
private:
    ModificationActivityDetector modificationDetector;
    CanaryDetector canaryDetector;
    StateManager stateManager;
    SecurityLogger logger;

public:
    SecurityEngine();
    SecurityDecision processRequest(const oops::Request& request);
};

#endif
