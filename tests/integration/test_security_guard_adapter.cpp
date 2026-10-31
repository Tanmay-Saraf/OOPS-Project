#include "integration/SecurityGuardAdapter.hpp"

#include <gtest/gtest.h>

namespace oops {

TEST(SecurityGuardAdapterTest, StatusReturnsCurrentSecurityState) {
    SecurityGuardAdapter guard("security_guard_adapter_test.log");

    Request request;
    request.command = Command::Status;

    GuardDecision decision = guard.check(request);

    EXPECT_TRUE(decision.allowed);
    EXPECT_EQ(decision.state, "RUNNING");
}

TEST(SecurityGuardAdapterTest, ProtocolCommandsAreRejectedBySecurityEngine) {
    SecurityGuardAdapter guard("security_guard_adapter_test.log");

    for (Command command : {Command::Ping, Command::Quit}) {
        Request request;
        request.command = command;

        GuardDecision decision = guard.check(request);

        EXPECT_FALSE(decision.allowed);
        EXPECT_EQ(decision.state, "RUNNING");
    }
}

TEST(SecurityGuardAdapterTest, CanaryTriggersLockdownAndBlocksWrites) {
    SecurityGuardAdapter guard("security_guard_adapter_test.log");

    Request attack;
    attack.command = Command::Set;
    attack.key = "canary_001";
    attack.value = "compromised";

    GuardDecision attackDecision = guard.check(attack);

    EXPECT_FALSE(attackDecision.allowed);
    EXPECT_EQ(attackDecision.state, "LOCKDOWN");

    Request read;
    read.command = Command::Get;
    read.key = "probe";

    GuardDecision readDecision = guard.check(read);

    EXPECT_TRUE(readDecision.allowed);
    EXPECT_EQ(readDecision.state, "LOCKDOWN");

    Request write;
    write.command = Command::Set;
    write.key = "probe";
    write.value = "changed";

    GuardDecision writeDecision = guard.check(write);

    EXPECT_FALSE(writeDecision.allowed);
    EXPECT_EQ(writeDecision.state, "LOCKDOWN");
}

}
