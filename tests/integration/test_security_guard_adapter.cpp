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

}
