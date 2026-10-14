
#include "../src/SecurityEngine.h"
#include <cassert>
#include <iostream>

int main() {
    SecurityEngine engine;
    Request request{};

    request.operation = Operation::SET;
    request.recordId = "canary_001";

    SecurityDecision decision = engine.processRequest(request);

    assert(!decision.allowed);
    assert(decision.currentState == SecurityState::LOCKDOWN);
    assert(decision.reason == "Attempt to modify or delete canary records");

    Request readRequest{};
    readRequest.operation = Operation::GET;
    readRequest.recordId = "normal_record";

    decision = engine.processRequest(readRequest);

    assert(decision.allowed);
    assert(decision.currentState == SecurityState::LOCKDOWN);

    Request writeRequest{};
    writeRequest.operation = Operation::SET;
    writeRequest.recordId = "normal_record";

    decision = engine.processRequest(writeRequest);

    assert(!decision.allowed);
    assert(decision.currentState == SecurityState::LOCKDOWN);

    std::cout << "All CanarySecurityEngine tests passed!\n";
}