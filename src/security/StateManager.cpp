#include "security/StateManager.h"

namespace oops {

SecurityState StateManager::getState() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return currentState;
}

void StateManager::markSuspected() {
    std::lock_guard<std::mutex> lock(stateMutex);

    if (currentState == SecurityState::RUNNING) {
        currentState = SecurityState::SUSPECTED;
    }
}

void StateManager::enterLockdown() {
    std::lock_guard<std::mutex> lock(stateMutex);
    currentState = SecurityState::LOCKDOWN;
}

}
