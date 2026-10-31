
#include "security/SecurityEngine.hpp"
#include "TestCheck.hpp"

#include <filesystem>
#include <iostream>

int main() {

#if defined(__linux__)

    const std::string failingLog = "/dev/full";

    if (!std::filesystem::exists(failingLog)) {
        std::cout << "SKIP: /dev/full is unavailable\n";
        return 0;
    }

    // TEST 1: Canary detection must deny even if logging fails.
    {
        oops::SecurityEngine engine(failingLog);

        oops::Request request{};
        request.command = oops::Command::Set;
        request.key = "canary_001";
        request.value = "trigger";

        const auto decision = engine.processRequest(request);

        TEST_CHECK(!decision.allowed);
        TEST_CHECK(
            decision.currentState == oops::SecurityState::LOCKDOWN
        );
    }

    // TEST 2: Modification activity must still be blocked.
    {
        oops::SecurityEngine engine(failingLog);

        oops::Request request{};
        request.command = oops::Command::Set;
        request.key = "ordinary_key";
        request.value = "ordinary_value";

        for (int i = 0; i < 99; ++i) {
            const auto decision = engine.processRequest(request);
            TEST_CHECK(decision.allowed);
        }

        const auto decision = engine.processRequest(request);

        TEST_CHECK(!decision.allowed);
        TEST_CHECK(
            decision.currentState == oops::SecurityState::SUSPECTED
        );
    }

    // TEST 3: Entropy-only alerts must be denied if logging fails.
    {
        oops::SecurityEngine engine(failingLog);

        oops::Request request{};
        request.command = oops::Command::Set;
        request.key = "ordinary_key";
        request.value =
            "aZ8!mQ2@xL5#pR9$wT4^nB7&cD6*eF09";

        const auto decision = engine.processRequest(request);

        TEST_CHECK(!decision.allowed);

        // Entropy alone should not trigger lockdown.
        TEST_CHECK(
            decision.currentState == oops::SecurityState::RUNNING
        );
    }
        // TEST 4: Correlated alerts must deny writes even if logging fails.
    {
        oops::SecurityEngine engine(failingLog);

        oops::Request request{};
        request.command = oops::Command::Set;
        request.key = "ordinary_key";
        request.value = "ordinary_value";

        // Build up modification activity without triggering the threshold.
        for (int i = 0; i < 99; ++i) {
            const auto decision = engine.processRequest(request);
            TEST_CHECK(decision.allowed);
        }

        // The 100th write also contains a high-entropy value.
        request.value =
            "aZ8!mQ2@xL5#pR9$wT4^nB7&cD6*eF09";

        const auto decision = engine.processRequest(request);

        TEST_CHECK(!decision.allowed);
        TEST_CHECK(
            decision.currentState == oops::SecurityState::SUSPECTED
        );
    }
    std::cout << "All SecurityLoggerFailure tests passed!\n";

#else

    std::cout
        << "SKIP: This test requires Linux /dev/full\n";

    return 77;

#endif

    return 0;
}
