#include "security/SecurityEngine.hpp"

#include "TestCheck.hpp"
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

int main() {
    oops::SecurityEngine engine;

    oops::Request request{};
    request.command = oops::Command::Get;
    request.key = "test_key";

    oops::SecurityDecision decision = engine.processRequest(request);
    TEST_CHECK(decision.allowed);
    TEST_CHECK(decision.currentState == oops::SecurityState::RUNNING);

    const auto expectDenied = [&engine](oops::Command command) {
        oops::Request input{};
        input.command = command;
        TEST_CHECK(!engine.processRequest(input).allowed);
    };

    expectDenied(oops::Command::Invalid);
    expectDenied(static_cast<oops::Command>(255));
    expectDenied(oops::Command::Ping);
    expectDenied(oops::Command::Quit);
    expectDenied(oops::Command::Save);
    expectDenied(oops::Command::Recover);

    TEST_CHECK(engine.processRequest(request).currentState ==
               oops::SecurityState::RUNNING);

    const auto missingParent =
        std::filesystem::temp_directory_path() /
        ("oops_missing_log_parent_" +
         std::to_string(
             std::chrono::steady_clock::now()
                 .time_since_epoch().count()
         ));
    std::filesystem::remove_all(missingParent);

    bool loggerInitializationFailed = false;
    try {
        oops::SecurityEngine invalidLoggerEngine(
            (missingParent / "security.log").string()
        );
    } catch (const std::runtime_error&) {
        loggerInitializationFailed = true;
    }

    TEST_CHECK(loggerInitializationFailed);
    std::filesystem::remove_all(missingParent);

    return 0;
}
