#include "security/SecurityEngine.hpp"

#include "TestCheck.hpp"

int main() {
    oops::SecurityEngine engine;

    oops::Request trigger{};
    trigger.command = oops::Command::Set;
    trigger.key = "canary_001";
    trigger.value = "trigger_lockdown";

    const oops::SecurityDecision triggerDecision =
        engine.processRequest(trigger);

    TEST_CHECK(!triggerDecision.allowed);
    TEST_CHECK(triggerDecision.currentState == oops::SecurityState::LOCKDOWN);

    oops::Request readRequest{};
    readRequest.command = oops::Command::Get;
    readRequest.key = "test_key";

    const oops::SecurityDecision getDecision =
        engine.processRequest(readRequest);
    TEST_CHECK(getDecision.allowed);
    TEST_CHECK(getDecision.currentState == oops::SecurityState::LOCKDOWN);

    const auto expectLockdownDenial = [&engine](
        oops::Command command
    ) {
        oops::Request request{};
        request.command = command;

        switch (command) {
            case oops::Command::Set:
                request.key = "test_key";
                request.value = "test_value";
                break;

            case oops::Command::Delete:
            case oops::Command::Exists:
            case oops::Command::History:
                request.key = "test_key";
                break;

            case oops::Command::GetVersion:
                request.key = "test_key";
                request.version = 1;
                break;

            default:
                break;
        }

        const oops::SecurityDecision decision =
            engine.processRequest(request);
        TEST_CHECK(!decision.allowed);
        TEST_CHECK(decision.currentState == oops::SecurityState::LOCKDOWN);
        TEST_CHECK(decision.reason == "Command denied during lockdown");
    };

    expectLockdownDenial(oops::Command::Exists);
    expectLockdownDenial(oops::Command::Keys);
    expectLockdownDenial(oops::Command::History);
    expectLockdownDenial(oops::Command::GetVersion);
    expectLockdownDenial(oops::Command::Status);
    expectLockdownDenial(oops::Command::Set);
    expectLockdownDenial(oops::Command::Delete);
    expectLockdownDenial(oops::Command::Save);
    expectLockdownDenial(oops::Command::Recover);
    expectLockdownDenial(oops::Command::Ping);
    expectLockdownDenial(oops::Command::Quit);

    oops::Request invalidRequest{};
    invalidRequest.command = oops::Command::Invalid;
    TEST_CHECK(!engine.processRequest(invalidRequest).allowed);

    return 0;
}
