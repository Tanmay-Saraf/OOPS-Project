#include "StateManager.h"

SecurityState StateManager::getState() const {
    return currentState;
}

void StateManager::setState(SecurityState newState) {
    currentState = newState;
}
