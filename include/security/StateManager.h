
#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H
#include <mutex>    

namespace oops {

enum class SecurityState {
    RUNNING,
    SUSPECTED,
    LOCKDOWN
};

class StateManager {
private:
    SecurityState currentState = SecurityState::RUNNING;
    mutable std::mutex stateMutex;

public:
    StateManager() = default;

    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;

    SecurityState getState() const;
    void setState(SecurityState newState);
};

}

#endif
