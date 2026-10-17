#pragma once

#include "common/Request.hpp"
#include "security/SecurityState.hpp"

#include <memory>
#include <string>

namespace oops {

struct SecurityDecision {
    bool allowed = false;
    std::string reason;
    SecurityState currentState = SecurityState::RUNNING;
};

class SecurityEngine {
public:
    explicit SecurityEngine(
        const std::string& logFile = "security.log"
    );
    ~SecurityEngine();

    SecurityEngine(const SecurityEngine&) = delete;
    SecurityEngine& operator=(const SecurityEngine&) = delete;

    SecurityDecision processRequest(const Request& request);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
