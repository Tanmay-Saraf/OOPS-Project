#include "security/StateManager.h"

#include "TestCheck.hpp"
#include <iostream>
#include <thread>
#include <vector>

int main() {
    oops::StateManager manager;
    TEST_CHECK(manager.getState() == oops::SecurityState::RUNNING);

    manager.markSuspected();
    TEST_CHECK(manager.getState() == oops::SecurityState::SUSPECTED);

    manager.markSuspected();
    TEST_CHECK(manager.getState() == oops::SecurityState::SUSPECTED);

    manager.enterLockdown();
    TEST_CHECK(manager.getState() == oops::SecurityState::LOCKDOWN);

    manager.markSuspected();
    TEST_CHECK(manager.getState() == oops::SecurityState::LOCKDOWN);

    oops::StateManager directLockdown;
    directLockdown.enterLockdown();
    directLockdown.markSuspected();
    TEST_CHECK(directLockdown.getState() == oops::SecurityState::LOCKDOWN);

    oops::StateManager concurrent;
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&concurrent, i]() {
            for (int j = 0; j < 100; ++j) {
                if ((i + j) % 2 == 0) {
                    concurrent.markSuspected();
                } else {
                    concurrent.enterLockdown();
                }
                (void)concurrent.getState();
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }
    TEST_CHECK(concurrent.getState() == oops::SecurityState::LOCKDOWN);

    std::cout << "All StateManager tests passed!\n";
    return 0;
}
