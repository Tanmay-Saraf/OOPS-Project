#include <chrono>
#include <thread>
#include <cassert>
#include <iostream>
#include "memory/MemoryStore.hpp"

int main() {
    MemoryStore store;

    // Test 1: Insert and retrieve a value.
    store.put("name", "Garvit");

    assert(store.get("name") != nullptr);
    assert(store.get("name")->value == "Garvit");

    // Test 2: Updating a key preserves its history.
    store.put("name", "Alex");

    assert(store.get("name")->value == "Alex");

    auto history = store.getHistory("name");

    assert(history.size() == 2);
    assert(history[0].value == "Garvit");
    assert(history[1].value == "Alex");

    // Test 3: A missing key returns nullptr.
    assert(store.get("missing") == nullptr);

    // Test 4: Deletion hides the key but preserves its history.
    assert(store.remove("name"));

    assert(store.get("name") == nullptr);
    assert(!store.remove("name"));
    assert(!store.remove("missing"));

    history = store.getHistory("name");

    assert(history.size() == 3);
    assert(history.back().operation == Operation::Delete);
        // Test 5: Retrieve historical values at different times.
    store.put("city", "Delhi");

    auto time1 = std::chrono::system_clock::now();

    std::this_thread::sleep_for(
        std::chrono::milliseconds(5)
    );

    store.put("city", "Mumbai");

    auto time2 = std::chrono::system_clock::now();

    std::this_thread::sleep_for(
        std::chrono::milliseconds(5)
    );

    store.remove("city");

    auto time3 = std::chrono::system_clock::now();

    auto oldVersion = store.getAtTime("city", time1);
    auto newVersion = store.getAtTime("city", time2);
    auto deletedVersion = store.getAtTime("city", time3);

    assert(oldVersion != nullptr);
    assert(oldVersion->value == "Delhi");

    assert(newVersion != nullptr);
    assert(newVersion->value == "Mumbai");

    assert(deletedVersion == nullptr);

    std::cout << "Time-travel tests passed!\n";

    std::cout << "Basic MemoryStore tests passed!\n";

    return 0;
}