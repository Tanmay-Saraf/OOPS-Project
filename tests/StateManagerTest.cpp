#include "security/StateManager.h"
#include <cassert>
#include <iostream>

int main() {
    oops::StateManager manager;

    assert(manager.getState() == oops::SecurityState::RUNNING);

    manager.setState(oops::SecurityState::SUSPECTED);
    assert(manager.getState() == oops::SecurityState::SUSPECTED);

    manager.setState(oops::SecurityState::LOCKDOWN);
    assert(manager.getState() == oops::SecurityState::LOCKDOWN);

    manager.setState(oops::SecurityState::RUNNING);
    assert(manager.getState() == oops::SecurityState::LOCKDOWN);

    std::cout << "All StateManager tests passed!\n";

    return 0;
}