#include "security/SecurityEngine.hpp"

#include "TestCheck.hpp"
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

int main() {
    oops::SecurityEngine engine;

    oops::Request request;
    request.command = oops::Command::Get;
    request.key = "test_key";

    oops::SecurityDecision decision = engine.processRequest(request);
    TEST_CHECK(decision.allowed);
    TEST_CHECK(decision.currentState == oops::SecurityState::RUNNING);

    request.command = oops::Command::Invalid;
    decision = engine.processRequest(request);
    TEST_CHECK(!decision.allowed);

    request.command = static_cast<oops::Command>(255);
    decision = engine.processRequest(request);
    TEST_CHECK(!decision.allowed);
    TEST_CHECK(decision.currentState == oops::SecurityState::RUNNING);

    request.command = oops::Command::Ping;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Quit;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Save;
    TEST_CHECK(!engine.processRequest(request).allowed);

    request.command = oops::Command::Recover;
    TEST_CHECK(!engine.processRequest(request).allowed);

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
