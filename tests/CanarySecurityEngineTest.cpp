#include "security/SecurityEngine.h"
#include "TestCheck.hpp"
#include <iostream>

int main() {
    oops::SecurityEngine engine;
    oops::Request request{};

    request.command = oops::Command::Set;
    request.key = "canary_001";

    oops::SecurityDecision decision = engine.processRequest(request);

    TEST_CHECK(!decision.allowed);
    TEST_CHECK(decision.currentState == oops::SecurityState::LOCKDOWN);
    TEST_CHECK(decision.reason == "Request denied by canary protection");

    oops::Request readRequest{};
    readRequest.command = oops::Command::Get;
    readRequest.key = "normal_record";

    decision = engine.processRequest(readRequest);

    TEST_CHECK(decision.allowed);
    TEST_CHECK(decision.currentState == oops::SecurityState::LOCKDOWN);

    oops::Request writeRequest{};
    writeRequest.command = oops::Command::Set;
    writeRequest.key = "normal_record";

    decision = engine.processRequest(writeRequest);

    TEST_CHECK(!decision.allowed);
    TEST_CHECK(decision.currentState == oops::SecurityState::LOCKDOWN);

    std::cout << "All CanarySecurityEngine tests passed!\n";
}