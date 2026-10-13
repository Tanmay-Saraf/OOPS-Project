
#include "../src/StateManager.h"
#include <cassert>
#include <iostream>

int main() {
    StateManager manager;

    assert(manager.getState() == SecurityState::RUNNING);

    manager.setState(SecurityState::SUSPECTED);
    assert(manager.getState() == SecurityState::SUSPECTED);

    manager.setState(SecurityState::LOCKDOWN);
    assert(manager.getState() == SecurityState::LOCKDOWN);

    manager.setState(SecurityState::RUNNING);
    assert(manager.getState() == SecurityState::RUNNING);

    std::cout << "All StateManager tests passed!\n";

    return 0;
}
