
#include "../src/SecurityEngine.h"
#include <cassert>
#include <iostream>

int main() {
    SecurityEngine engine;
    Request request{};

    request.operation = Operation::GET;

    SecurityDecision decision = engine.processRequest(request);
    assert(decision.allowed);

    request.operation = Operation::SET;

    for (int i = 0; i < 100; i++) {
        decision = engine.processRequest(request);
    }

    assert(!decision.allowed);
    assert(decision.currentState == SecurityState::SUSPECTED);

    std::cout << "All SecurityEngine tests passed!\n";

    return 0;
}
