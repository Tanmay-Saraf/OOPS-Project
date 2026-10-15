
#include "../src/SecurityEngine.h"
#include <cassert>
#include <iostream>

int main() {
    SecurityEngine engine;
    oops::Request request{};

    request.command = oops::Command::Set;
    request.key = "canary_001";

    SecurityDecision decision = engine.processRequest(request);

    assert(!decision.allowed);
    assert(decision.currentState == SecurityState::LOCKDOWN);
    assert(decision.reason == "Attempt to modify or delete canary records");

    oops::Request readRequest{};
    readRequest.command = oops::Command::Get;
    readRequest.key = "normal_record";

    decision = engine.processRequest(readRequest);

    assert(decision.allowed);
    assert(decision.currentState == SecurityState::LOCKDOWN);

    oops::Request writeRequest{};
    writeRequest.command = oops::Command::Set;
    writeRequest.key = "normal_record";

    decision = engine.processRequest(writeRequest);

    assert(!decision.allowed);
    assert(decision.currentState == SecurityState::LOCKDOWN);

    std::cout << "All CanarySecurityEngine tests passed!\n";
}