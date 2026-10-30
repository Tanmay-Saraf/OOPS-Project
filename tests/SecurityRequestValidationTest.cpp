#include "security/SecurityEngine.hpp"

#include "TestCheck.hpp"

int main() {
    oops::SecurityEngine engine;

    oops::Request validGet{};
    validGet.command = oops::Command::Get;
    validGet.key = "record";
    TEST_CHECK(engine.processRequest(validGet).allowed);

    oops::Request validSet{};
    validSet.command = oops::Command::Set;
    validSet.key = "record";
    validSet.value = "value";
    TEST_CHECK(engine.processRequest(validSet).allowed);

    oops::Request malformed{};
    malformed.command = oops::Command::Get;
    TEST_CHECK(!engine.processRequest(malformed).allowed);

    malformed = {};
    malformed.command = oops::Command::Get;
    malformed.key = "two words";
    TEST_CHECK(!engine.processRequest(malformed).allowed);

    malformed = {};
    malformed.command = oops::Command::Set;
    malformed.value = "value";
    TEST_CHECK(!engine.processRequest(malformed).allowed);

    malformed = {};
    malformed.command = oops::Command::Set;
    malformed.key = "record";
    TEST_CHECK(!engine.processRequest(malformed).allowed);

    malformed.value = " \t \t";
    TEST_CHECK(!engine.processRequest(malformed).allowed);

    malformed = {};
    malformed.command = oops::Command::Delete;
    malformed.key = "record";
    malformed.value = "unexpected";
    TEST_CHECK(!engine.processRequest(malformed).allowed);

    malformed = {};
    malformed.command = oops::Command::Keys;
    malformed.key = "unexpected";
    TEST_CHECK(!engine.processRequest(malformed).allowed);

    malformed = {};
    malformed.command = oops::Command::Status;
    malformed.version = 1;
    TEST_CHECK(!engine.processRequest(malformed).allowed);

    malformed = {};
    malformed.command = static_cast<oops::Command>(255);
    TEST_CHECK(!engine.processRequest(malformed).allowed);

    oops::Request validGetVersion{};
    validGetVersion.command = oops::Command::GetVersion;
    validGetVersion.key = "record";
    validGetVersion.version = 1;
    TEST_CHECK(engine.processRequest(validGetVersion).allowed);

    TEST_CHECK(engine.processRequest(validGet).currentState ==
               oops::SecurityState::RUNNING);

    return 0;
}
