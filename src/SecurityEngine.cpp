#include "SecurityEngine.h"

SecurityEngine::SecurityEngine()
    : logger("security.log") {}

SecurityDecision SecurityEngine::processRequest(const Request& request) {
    SecurityState state = stateManager.getState();

    if (state == SecurityState::LOCKDOWN) {
        if (request.operation == Operation::GET) {
            return {true, "Read allowed during lockdown", state};
        }

        logger.logEvent("REQUEST_BLOCKED", "Database is in lockdown");

        return {false, "Database is in lockdown", state};
    }

    DetectionResult canaryResult = canaryDetector.analyze(request);

    if (canaryResult.Suspicious_flag) {
        stateManager.setState(SecurityState::LOCKDOWN);

        logger.logEvent("CANARY_ALERT", canaryResult.reason);

        return {
            false,
            canaryResult.reason,
            stateManager.getState()
        };
    }

    DetectionResult modificationResult =
        modificationDetector.analyze(request);

    if (modificationResult.Suspicious_flag) {
        stateManager.setState(SecurityState::SUSPECTED);

        logger.logEvent("MODIFICATION_ALERT", modificationResult.reason);

        return {
            false,
            modificationResult.reason,
            stateManager.getState()
        };
    }

    return {true, "Request allowed", stateManager.getState()};
}
