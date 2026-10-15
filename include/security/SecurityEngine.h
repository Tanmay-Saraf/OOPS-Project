#ifndef SECURITY_ENGINE_H
#define SECURITY_ENGINE_H

#include "common/Request.hpp"
#include "DetectionResult.h"
#include "ModificationActivityDetector.h"
#include "CanaryDetector.h"
#include "EntropyDetector.h"
#include "StateManager.h"
#include "SecurityLogger.h"
#include <mutex>
#include <string>

namespace oops {

struct SecurityDecision {
    bool allowed = false;
    std::string reason;
    SecurityState currentState = SecurityState::RUNNING;
};

class SecurityEngine {
private:
    std::mutex requestMutex;
    ModificationActivityDetector modificationDetector;
    CanaryDetector canaryDetector;
    EntropyDetector entropyDetector;
    StateManager stateManager;
    SecurityLogger logger;

public:
    SecurityEngine();
    SecurityDecision processRequest(const Request& request);
};

}

#endif