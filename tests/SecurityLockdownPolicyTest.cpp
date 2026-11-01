
#include "security/SecurityEngine.hpp"

#include "TestCheck.hpp"

int main() {
    oops::SecurityEngine engine;

    oops::Request trigger;
    trigger.command = oops::Command::Set;
    trigger.key = "canary_001";
    trigger.value = "trigger_lockdown";

    oops::SecurityDecision triggerDecision =
        engine.processRequest(trigger);

    TEST_CHECK(!triggerDecision.allowed);
    TEST_CHECK(triggerDecision.currentState == oops::SecurityState::LOCKDOWN);

    oops::Request request;
    request.key = "test_key";

    request.command = oops::Command::Get;
    oops::SecurityDecision getDecision = engine.processRequest(request);
    TEST_CHECK(getDecision.allowed);
    TEST_CHECK(getDecision.currentState == oops::SecurityState::LOCKDOWN);

    request.command = oops::Command::Exists;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Keys;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::History;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::GetVersion;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Status;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Set;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Delete;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Save;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Recover;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Ping;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Quit;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Invalid;
    TEST_CHECK(!engine.processRequest(request).allowed);

    return 0;
}
