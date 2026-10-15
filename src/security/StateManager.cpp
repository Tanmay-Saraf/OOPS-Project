
#include "security/StateManager.h"

namespace oops {

SecurityState StateManager::getState() const {
    return currentState;
}

void StateManager::setState(SecurityState newState) {
    currentState = newState;
}

}
