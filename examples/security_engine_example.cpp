#include "security/SecurityEngine.hpp"

#include <iostream>

int main() {
    oops::SecurityEngine engine("security_example.log");

    oops::Request initialRead{};
    initialRead.command = oops::Command::Get;
    initialRead.key = "demo_key";

    const oops::SecurityDecision readDecision =
        engine.processRequest(initialRead);

    oops::Request canaryWrite{};
    canaryWrite.command = oops::Command::Set;
    canaryWrite.key = "canary_001";
    canaryWrite.value = "tripwire";

    const oops::SecurityDecision canaryDecision =
        engine.processRequest(canaryWrite);

    oops::Request lockdownRead{};
    lockdownRead.command = oops::Command::Get;
    lockdownRead.key = "demo_key";

    const oops::SecurityDecision lockdownReadDecision =
        engine.processRequest(lockdownRead);

    oops::Request lockdownWrite{};
    lockdownWrite.command = oops::Command::Set;
    lockdownWrite.key = "ordinary_key";
    lockdownWrite.value = "ordinary_value";

    const oops::SecurityDecision lockdownWriteDecision =
        engine.processRequest(lockdownWrite);

    std::cout << "Initial GET allowed: " << std::boolalpha
              << readDecision.allowed << '\n';
    std::cout << "Canary write allowed: " << canaryDecision.allowed << '\n';
    std::cout << "State after canary: "
              << (canaryDecision.currentState == oops::SecurityState::LOCKDOWN
                      ? "LOCKDOWN" : "unexpected") << '\n';
    std::cout << "GET during lockdown allowed: "
              << lockdownReadDecision.allowed << '\n';
    std::cout << "SET during lockdown allowed: "
              << lockdownWriteDecision.allowed << '\n';

    const bool passed =
        readDecision.allowed &&
        !canaryDecision.allowed &&
        canaryDecision.currentState == oops::SecurityState::LOCKDOWN &&
        lockdownReadDecision.allowed &&
        !lockdownWriteDecision.allowed;

    return passed ? 0 : 1;
}
