
#include "security/SecurityEngine.h"
#include "TestCheck.hpp"
#include <iostream>
#include <atomic>
#include <thread>
#include <vector>

using namespace oops;

int main() {
    SecurityEngine engine;
    Request request{};

    request.command = Command::Get;
    SecurityDecision decision = engine.processRequest(request);
    TEST_CHECK(decision.allowed);

    request.command = Command::Set;
    request.key = "normal_record";
    request.value = "normal";

    for (int i = 0; i < 100; i++) {
        decision = engine.processRequest(request);
    }

    TEST_CHECK(!decision.allowed);
    TEST_CHECK(decision.currentState == SecurityState::SUSPECTED);

    request.key = "canary_001";
    decision = engine.processRequest(request);

    TEST_CHECK(!decision.allowed);
    TEST_CHECK(decision.currentState == SecurityState::LOCKDOWN);
    TEST_CHECK(decision.reason == "Request denied by canary protection");

    SecurityEngine entropyOnlyEngine;
    Request entropyRequest{};

    entropyRequest.command = Command::Set;
    entropyRequest.key = "normal_record";
    entropyRequest.value = "aZ8!mQ2@xL5#pR9$wT4^nB7&cD6*eF0";

    SecurityDecision entropyDecision =
        entropyOnlyEngine.processRequest(entropyRequest);

    TEST_CHECK(entropyDecision.allowed);
    TEST_CHECK(entropyDecision.currentState == SecurityState::RUNNING);
    TEST_CHECK(entropyDecision.reason ==
           "Request allowed; entropy alert recorded");

    SecurityEngine combinedEngine;
    Request combinedRequest{};

    combinedRequest.command = Command::Set;
    combinedRequest.key = "normal_record";
    combinedRequest.value = "normal";

    for (int i = 0; i < 99; i++) {
        entropyDecision = combinedEngine.processRequest(combinedRequest);
        TEST_CHECK(entropyDecision.allowed);
    }

    combinedRequest.value = "aZ8!mQ2@xL5#pR9$wT4^nB7&cD6*eF0";
    entropyDecision = combinedEngine.processRequest(combinedRequest);

    TEST_CHECK(!entropyDecision.allowed);
    TEST_CHECK(entropyDecision.currentState == SecurityState::LOCKDOWN);
    TEST_CHECK(entropyDecision.reason ==
           "Request denied due to correlated security alerts");

    SecurityEngine concurrentEngine;
    std::atomic<int> allowedRequests{0};
    std::atomic<int> blockedRequests{0};
    std::vector<std::thread> workers;

    for (int i = 0; i < 4; i++) {
        workers.emplace_back([&, i]() {
            for (int j = 0; j < 25; j++) {
                Request concurrentRequest{};
                concurrentRequest.command = Command::Set;
                concurrentRequest.key =
                    "concurrent_" + std::to_string(i) + "_" +
                    std::to_string(j);
                concurrentRequest.value = "normal";

                SecurityDecision result =
                    concurrentEngine.processRequest(concurrentRequest);

                if (result.allowed) {
                    ++allowedRequests;
                } else {
                    ++blockedRequests;
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    TEST_CHECK(allowedRequests + blockedRequests == 100);
    TEST_CHECK(blockedRequests >= 1);

    Request statusRequest{};
    statusRequest.command = Command::Status;
    SecurityDecision status =
        concurrentEngine.processRequest(statusRequest);

    TEST_CHECK(status.currentState == SecurityState::SUSPECTED);

    std::cout << "All SecurityEngine tests passed!\n";
    return 0;
}
