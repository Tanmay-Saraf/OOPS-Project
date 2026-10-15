
#include "security/StateManager.h"

namespace oops {

SecurityState StateManager::getState() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return currentState;
}

void StateManager::setState(SecurityState newState) {
    std::lock_guard<std::mutex> lock(stateMutex);

    if (newState == currentState) {
        return;
    }

    if (currentState == SecurityState::RUNNING) {
        if (newState == SecurityState::SUSPECTED ||
            newState == SecurityState::LOCKDOWN) {
            currentState = newState;
        }
        return;
    }

    if (currentState == SecurityState::SUSPECTED &&
        newState == SecurityState::LOCKDOWN) {
        currentState = newState;
    }
}

}
