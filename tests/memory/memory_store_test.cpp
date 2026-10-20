#include <cassert>
#include <iostream>
#include <chrono>
#include <thread>
#include "memory/MemoryStore.hpp"

int main(){
    MemoryStore store;

    // Test 1: Insert and retrieve a value
    store.put("name", "Garvit");

    assert(store.get("name")!=nullptr);
    assert(store.get("name")->value=="Garvit");

    // Test 2: Update a key and preserve history
    store.put("name", "Alex");
    assert(store.exists("name"));
assert(!store.exists("missing"));
    assert(store.get("name")->value=="Alex");

    auto history=store.getHistory("name");

    assert(history.size()==2);
    assert(history[0].value=="Garvit");
    assert(history[1].value=="Alex");

    store.put("zebra", "animal");
store.put("apple", "fruit");

auto currentKeys=store.keys();

assert(currentKeys.size()==3);
assert(currentKeys[0]=="apple");
assert(currentKeys[1]=="name");
assert(currentKeys[2]=="zebra");
    // Test 3: Retrieve a missing key
    assert(store.get("missing")==nullptr);

    // Test 4: Delete an existing key
    bool removedName=store.remove("name");
    assert(removedName);

    assert(store.get("name")==nullptr);

    history=store.getHistory("name");

    assert(history.size()==3);
    assert(history.back().operation==Operation::Delete);

    // Test 5: Delete an already deleted key
    bool removedAgain=store.remove("name");
    assert(!removedAgain);

    history=store.getHistory("name");
    assert(history.size()==3);

    currentKeys=store.keys();

assert(currentKeys.size()==2);
assert(currentKeys[0]=="apple");
assert(currentKeys[1]=="zebra");
    // Test 6: Delete a key that never existed
    bool removedMissing=store.remove("missing");
    assert(!removedMissing);
    assert(!store.exists("name"));
    // Test 7: Historical lookups
    store.put("city", "Delhi");

    auto time1=std::chrono::system_clock::now();

    std::this_thread::sleep_for(
        std::chrono::milliseconds(5)
    );

    store.put("city", "Mumbai");

    auto time2=std::chrono::system_clock::now();

    std::this_thread::sleep_for(
        std::chrono::milliseconds(5)
    );

    bool removedCity=store.remove("city");
    assert(removedCity);

    auto time3=std::chrono::system_clock::now();

    auto oldVersion=store.getAtTime("city", time1);
    auto newVersion=store.getAtTime("city", time2);
    auto deletedVersion=store.getAtTime("city", time3);

    assert(oldVersion!=nullptr);
    assert(oldVersion->value=="Delhi");

    assert(newVersion!=nullptr);
    assert(newVersion->value=="Mumbai");

    assert(deletedVersion==nullptr);

    std::cout<<"Time-travel tests passed!\n";
    std::cout<<"Basic MemoryStore tests passed!\n";

    return 0;
}