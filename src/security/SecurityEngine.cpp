
#include "security/SecurityEngine.h"

namespace oops {
SecurityEngine::SecurityEngine() : logger("security.log") {}

SecurityDecision SecurityEngine::processRequest(const Request& request) {
    SecurityDecision decision;

    decision.currentState = stateManager.getState();

    if (request.command == Command::Get ||
        request.command == Command::Exists ||
        request.command == Command::Keys ||
        request.command == Command::History ||
        request.command == Command::GetVersion ||
        request.command == Command::Ping ||
        request.command == Command::Quit ||
        request.command == Command::Status) {
        decision.allowed = true;
        decision.reason = "Read or safe command";
        return decision;
    }

    if (request.command == Command::Save ||
        request.command == Command::Recover ||
        request.command == Command::Invalid) {
        decision.allowed = false;
        decision.reason = "Command denied by security policy";
        return decision;
    }

    if (stateManager.getState() != SecurityState::RUNNING) {
        decision.allowed = false;
        decision.reason = "Writes blocked by security state";
        return decision;
    }

    DetectionResult canaryResult = canaryDetector.analyze(request);

    if (canaryResult.Suspicious_flag) {
        stateManager.setState(SecurityState::LOCKDOWN);
        logger.logEvent("CRITICAL", canaryResult.reason);
        decision.allowed = false;
        decision.reason = canaryResult.reason;
        decision.currentState = stateManager.getState();
        return decision;
    }

    DetectionResult activityResult = modificationDetector.analyze(request);

    if (activityResult.Suspicious_flag) {
        stateManager.setState(SecurityState::SUSPECTED);
        logger.logEvent("HIGH", activityResult.reason);
        decision.allowed = false;
        decision.reason = activityResult.reason;
        decision.currentState = stateManager.getState();
        return decision;
    }

    decision.allowed = true;
    decision.reason = "Request allowed";
    decision.currentState = stateManager.getState();

    return decision;
}

}
