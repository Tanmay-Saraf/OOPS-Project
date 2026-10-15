#include "security/SecurityEngine.h"
#include <exception>
namespace oops {

SecurityEngine::SecurityEngine() : logger("security.log") {}

SecurityDecision SecurityEngine::processRequest(const Request& request) {
    std::lock_guard<std::mutex> lock(requestMutex);                                                                                         
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
        decision.reason = "Command denied by security policy";
        return decision;
    }

    DetectionResult canaryResult = canaryDetector.analyze(request);

    if (canaryResult.Suspicious_flag) {
        stateManager.setState(SecurityState::LOCKDOWN);
        decision.allowed = false;
        decision.reason = canaryResult.reason;

        try {
            logger.logEvent("CRITICAL", canaryResult.reason);
        } catch (const std::exception& e) {
            decision.reason += " | Logging failure: ";
            decision.reason += e.what();
        }

        decision.currentState = stateManager.getState();
        return decision;
    }

    if (stateManager.getState() != SecurityState::RUNNING) {
        decision.reason = "Writes blocked by security state";
        decision.currentState = stateManager.getState();
        return decision;
    }

    DetectionResult activityResult =
        modificationDetector.analyze(request);

    DetectionResult entropyResult =
        entropyDetector.analyze(request);

    if (activityResult.Suspicious_flag &&
        entropyResult.Suspicious_flag) {
        stateManager.setState(SecurityState::LOCKDOWN);
        try {
            logger.logEvent("CRITICAL",
                "High modification rate combined with high-entropy value");
        } catch (const std::exception& e) {
            decision.reason += " | Logging failure: ";
            decision.reason += e.what();
        }

        decision.reason =
            "Combined modification-rate and entropy alerts";
        decision.currentState = stateManager.getState();
        return decision;
    }

    if (activityResult.Suspicious_flag) {
        stateManager.setState(SecurityState::SUSPECTED);
        try {
            logger.logEvent("HIGH", activityResult.reason);
        } catch (const std::exception& e) {
            decision.reason += " | Logging failure: ";
            decision.reason += e.what();
        }

        decision.reason = activityResult.reason;
        decision.currentState = stateManager.getState();
        return decision;
    }

    if (entropyResult.Suspicious_flag) {
        decision.allowed = true;
        decision.reason = "Request allowed; high-entropy value observed";

        try {
            logger.logEvent("ENTROPY_ALERT", entropyResult.reason);
        } catch (const std::exception& e) {
            decision.reason += " | Logging failure: ";
            decision.reason += e.what();
        }

        decision.currentState = stateManager.getState();
        return decision;
    }

    decision.allowed = true;
    decision.reason = "Request allowed";
    decision.currentState = stateManager.getState();

    return decision;
}

}