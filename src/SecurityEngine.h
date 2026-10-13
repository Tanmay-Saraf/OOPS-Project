#ifndef SECURITY_ENGINE_H
#define SECURITY_ENGINE_H
#include "Request.h"
#include "DetectionResult.h"
#include "ModificationActivityDetector.h"
#include "StateManager.h"
struct SecurityDecision {
    bool allowed;
    std::string reason;
    SecurityState currentState;
};
class SecurityEngine {
private:
    ModificationActivityDetector modificationDetector;
    StateManager stateManager;

public:
    SecurityDecision processRequest(const Request& request);
};

#endif
