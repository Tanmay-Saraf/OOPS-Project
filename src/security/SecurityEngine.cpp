#include "security/SecurityEngine.hpp"

#include "security/CanaryDetector.h"
#include "security/EntropyDetector.h"
#include "security/ModificationActivityDetector.h"
#include "security/SecurityLogger.h"
#include "security/StateManager.h"

#include <mutex>
#include <string>

namespace oops {

class SecurityEngine::Impl {
private:
    std::mutex requestMutex;
    ModificationActivityDetector modificationDetector;
    CanaryDetector canaryDetector;
    EntropyDetector entropyDetector;
    StateManager stateManager;
    SecurityLogger logger;

    bool tryLogEvent(const std::string& event,
                     const std::string& detail) noexcept {
        try {
            logger.logEvent(event, detail);
            return true;
        } catch (...) {
            return false;
        }
    }

    SecurityDecision deny(const std::string& safeReason,
                          const std::string& logDetail,
                          const std::string& event = "REQUEST_DENIED") {
        SecurityDecision decision;
        decision.allowed = false;
        decision.reason = safeReason;
        decision.currentState = stateManager.getState();
        tryLogEvent(event, logDetail);
        return decision;
    }

public:
    explicit Impl(const std::string& logFile) : logger(logFile) {}

    SecurityDecision processRequest(const Request& request) {
        std::lock_guard<std::mutex> lock(requestMutex);

        SecurityDecision decision;
        decision.currentState = stateManager.getState();

        // In lockdown, GET is the only command permitted by the engine.
        if (decision.currentState == SecurityState::LOCKDOWN) {
            if (request.command == Command::Get) {
                decision.allowed = true;
                decision.reason = "GET allowed during lockdown";
                return decision;
            }

            return deny(
                "Command denied during lockdown",
                "Command denied because the security state is LOCKDOWN"
            );
        }

        switch (request.command) {
            case Command::Get:
            case Command::Exists:
            case Command::Keys:
            case Command::History:
            case Command::GetVersion:
            case Command::Status:
                decision.allowed = true;
                decision.reason = "Read request allowed";
                return decision;

            case Command::Ping:
            case Command::Quit:
                return deny(
                    "Protocol-only command must be handled by Network Guard",
                    "Protocol-only command was passed to SecurityEngine"
                );

            case Command::Save:
            case Command::Recover:
            case Command::Invalid:
                return deny(
                    "Command denied by security policy",
                    "Privileged or invalid command denied"
                );

            case Command::Set:
            case Command::Delete:
                break;

            default:
                return deny(
                    "Unknown command denied by security policy",
                    "Unrecognized command enum value denied"
                );
        }

        DetectionResult canaryResult = canaryDetector.analyze(request);

        if (canaryResult.Suspicious_flag) {
            stateManager.enterLockdown();
            decision.allowed = false;
            decision.reason = "Request denied by canary protection";
            decision.currentState = stateManager.getState();
            tryLogEvent("CRITICAL", canaryResult.reason);
            return decision;
        }

        if (stateManager.getState() != SecurityState::RUNNING) {
            return deny(
                "Write denied by current security state",
                "Write denied because the security state is not RUNNING"
            );
        }

        DetectionResult activityResult = modificationDetector.analyze(request);
        DetectionResult entropyResult = entropyDetector.analyze(request);

        if (activityResult.Suspicious_flag && entropyResult.Suspicious_flag) {
            stateManager.enterLockdown();
            decision.allowed = false;
            decision.reason =
                "Request denied due to correlated security alerts";
            decision.currentState = stateManager.getState();
            tryLogEvent(
                "CRITICAL",
                "High modification activity combined with a high-entropy value"
            );
            return decision;
        }

        if (activityResult.Suspicious_flag) {
            stateManager.markSuspected();
            decision.allowed = false;
            decision.reason =
                "Request denied due to abnormal modification activity";
            decision.currentState = stateManager.getState();
            tryLogEvent("HIGH", activityResult.reason);
            return decision;
        }

        if (entropyResult.Suspicious_flag) {
            const bool logged = tryLogEvent(
                "ENTROPY_ALERT", entropyResult.reason
            );

            decision.allowed = logged;
            decision.reason = logged
                ? "Request allowed; entropy alert recorded"
                : "Request denied because the security alert could not be logged";
            decision.currentState = stateManager.getState();
            return decision;
        }

        decision.allowed = true;
        decision.reason = "Request allowed";
        decision.currentState = stateManager.getState();
        return decision;
    }
};

SecurityEngine::SecurityEngine(const std::string& logFile)
    : impl_(std::make_unique<Impl>(logFile)) {}

SecurityEngine::~SecurityEngine() = default;

SecurityDecision SecurityEngine::processRequest(const Request& request) {
    return impl_->processRequest(request);
}

}
