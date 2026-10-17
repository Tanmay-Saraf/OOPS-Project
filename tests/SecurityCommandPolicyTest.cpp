#include "security/SecurityEngine.hpp"

#include "TestCheck.hpp"
#include <iostream>
#include <utility>
#include <vector>

int main() {
    oops::SecurityEngine engine;
    oops::Request request{};
    request.key = "ordinary_key";
    request.value = "short_value";

    const std::vector<std::pair<oops::Command, bool>> policy = {
        {oops::Command::Get, true},
        {oops::Command::Exists, true},
        {oops::Command::Keys, true},
        {oops::Command::History, true},
        {oops::Command::GetVersion, true},
        {oops::Command::Status, true},
        {oops::Command::Ping, false},
        {oops::Command::Quit, false},
        {oops::Command::Save, false},
        {oops::Command::Recover, false},
        {oops::Command::Invalid, false},
        {static_cast<oops::Command>(255), false},
        {oops::Command::Set, true},
        {oops::Command::Delete, true}
    };

    for (const auto& [command, expectedAllowed] : policy) {
        request.command = command;
        const oops::SecurityDecision decision = engine.processRequest(request);
        TEST_CHECK(decision.allowed == expectedAllowed);
        TEST_CHECK(decision.currentState == oops::SecurityState::RUNNING);
    }

    std::cout << "All SecurityCommandPolicy tests passed!\n";
    return 0;
}
