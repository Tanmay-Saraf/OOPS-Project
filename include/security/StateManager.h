#pragma once

#include "security/SecurityState.hpp"

#include <mutex>

namespace oops {

class StateManager {
private:
    SecurityState currentState = SecurityState::RUNNING;
    mutable std::mutex stateMutex;

public:
    StateManager() = default;

    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;

    SecurityState getState() const;

    // State transitions are intentionally monotonic. There is no public
    // transition back to RUNNING; recovery needs a trusted future API.
    void markSuspected();
    void enterLockdown();
};

}
