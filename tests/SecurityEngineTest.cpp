#include "security/SecurityEngine.h"
#include <cassert>
#include <iostream>

int main() {
    SecurityEngine engine;
    oops::Request request{};

    request.command = oops::Command::Get;

    SecurityDecision decision = engine.processRequest(request);
    assert(decision.allowed);

    request.command = oops::Command::Set;
    request.key = "normal_record";

    for (int i = 0; i < 100; i++) {                             
        decision = engine.processRequest(request);
    }

    assert(!decision.allowed);
    assert(decision.currentState == SecurityState::SUSPECTED);

    std::cout << "All SecurityEngine tests passed!\n";

    return 0;
}
