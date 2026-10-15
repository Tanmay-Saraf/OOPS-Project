#include "security/SecurityEngine.h"
#include <cassert>
#include <iostream>

int main() {
    oops::SecurityEngine engine;
    oops::Request request{};

    request.command = oops::Command::Set;
    request.key = "canary_001";

    oops::SecurityDecision decision = engine.processRequest(request);

    assert(!decision.allowed);
    assert(decision.currentState == oops::SecurityState::LOCKDOWN);
    assert(decision.reason == "Attempt to modify or delete canary records");

    oops::Request readRequest{};
    readRequest.command = oops::Command::Get;
    readRequest.key = "normal_record";

    decision = engine.processRequest(readRequest);

    assert(decision.allowed);
    assert(decision.currentState == oops::SecurityState::LOCKDOWN);

    oops::Request writeRequest{};
    writeRequest.command = oops::Command::Set;
    writeRequest.key = "normal_record";

    decision = engine.processRequest(writeRequest);

    assert(!decision.allowed);
    assert(decision.currentState == oops::SecurityState::LOCKDOWN);

    std::cout << "All CanarySecurityEngine tests passed!\n";
}