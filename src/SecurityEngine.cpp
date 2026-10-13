#include "SecurityEngine.h"

SecurityDecision SecurityEngine::processRequest(const Request& request) {
    SecurityState state = stateManager.getState();

    if (state == SecurityState::LOCKDOWN) {
        if (request.operation == Operation::GET) {
            return {true, "Read allowed during lockdown", state};
        }

        return {false, "Database is in lockdown", state};
    }

    DetectionResult result = modificationDetector.analyze(request);

    if (result.Suspicious_flag) {
        stateManager.setState(SecurityState::SUSPECTED);

        return {
            false,
            result.reason,
            stateManager.getState()
        };
    }

    return {true, "Request allowed", stateManager.getState()};
}
