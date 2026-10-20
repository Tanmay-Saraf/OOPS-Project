
#pragma once

#include "network/RequestGuard.hpp"
#include "security/SecurityEngine.hpp"

#include <string>

namespace oops {

    class SecurityGuardAdapter : public RequestGuard {

    public:

        explicit SecurityGuardAdapter(const std::string& logFile = "security.log") : engine_(logFile) {}

        GuardDecision check(const Request& request) override {

            SecurityDecision decision = engine_.processRequest(request);

            return GuardDecision{decision.allowed, decision.reason};

        }

    private:

        SecurityEngine engine_;

    };

}
