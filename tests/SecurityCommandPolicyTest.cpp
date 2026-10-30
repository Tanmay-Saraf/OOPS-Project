#include "security/SecurityEngine.hpp"

#include "TestCheck.hpp"
#include <iostream>
#include <utility>
#include <vector>

int main() {
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
        oops::SecurityEngine engine;
        oops::Request request{};
        request.command = command;

        switch (command) {
            case oops::Command::Set:
                request.key = "ordinary_key";
                request.value = "ordinary_value";
                break;

            case oops::Command::Get:
            case oops::Command::Delete:
            case oops::Command::Exists:
            case oops::Command::History:
                request.key = "ordinary_key";
                break;

            case oops::Command::GetVersion:
                request.key = "ordinary_key";
                request.version = 1;
                break;

            default:
                break;
        }

        const oops::SecurityDecision decision =
            engine.processRequest(request);
        TEST_CHECK(decision.allowed == expectedAllowed);
        TEST_CHECK(decision.currentState == oops::SecurityState::RUNNING);
    }

    std::cout << "All SecurityCommandPolicy tests passed!\n";
    return 0;
}
