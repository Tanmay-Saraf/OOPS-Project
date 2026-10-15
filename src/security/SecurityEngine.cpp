#include "security/SecurityEngine.h"

SecurityEngine::SecurityEngine()
    : logger("security.log") {}

SecurityDecision SecurityEngine::processRequest(
    const oops::Request& request
) {
    SecurityState state = stateManager.getState();

    auto deny = [&](const std::string& reason) {
        logger.logEvent("REQUEST_BLOCKED", reason);
        return SecurityDecision{false, reason, stateManager.getState()};
    };

    switch (request.command) {
        case oops::Command::Invalid:
            return deny("Invalid command");

        case oops::Command::Save:
            return deny("Save denied: authorization not implemented");

        case oops::Command::Recover:
            return deny("Recovery denied: authorization not implemented");

        case oops::Command::Ping:
        case oops::Command::Quit:
        case oops::Command::Status:
        case oops::Command::Get:
        case oops::Command::Exists:
        case oops::Command::Keys:
        case oops::Command::History:
        case oops::Command::GetVersion:
            return {true, "Request allowed", state};

        case oops::Command::Set:
        case oops::Command::Delete:
            break;
    }

    if (state != SecurityState::RUNNING) {
        return deny("Writes denied while security state is not RUNNING");
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
