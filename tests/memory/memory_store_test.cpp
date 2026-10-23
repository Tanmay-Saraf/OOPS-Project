#include <cassert>
#include <iostream>
#include <chrono>
#include <thread>
#include <limits>
#include <stdexcept>
#include "memory/MemoryStore.hpp"

int main(){
    MemoryStore store;

    // Test 1: Insert and retrieve a value
    store.put("name", "Garvit");

    auto result=store.get("name");
    assert(result.has_value());
    assert(result->value=="Garvit");
    assert(store.exists("name"));
    assert(!store.exists("missing"));

    // Test 2: Update a key and preserve history
    store.put("zebra", "animal");
    store.put("apple", "fruit");
    store.put("name", "Alex");

    result=store.get("name");
    assert(result.has_value());
    assert(result->value=="Alex");

    auto history=store.getHistory("name");

    assert(history.size()==2);
    assert(history[0].value=="Garvit");
    assert(history[1].value=="Alex");
    assert(history[0].id<history[1].id);

    // Test 3: Retrieve a specific version
    auto firstVersion=store.getVersion("name", history[0].id);
    auto secondVersion=store.getVersion("name", history[1].id);

    assert(firstVersion.has_value());
    assert(firstVersion->value=="Garvit");
    assert(firstVersion->operation==Operation::Put);

    assert(secondVersion.has_value());
    assert(secondVersion->value=="Alex");
    assert(secondVersion->operation==Operation::Put);

    auto missingVersion=store.getVersion("name", 999999);
    assert(!missingVersion.has_value());

    auto wrongKeyVersion=store.getVersion("apple", history[0].id);
    assert(!wrongKeyVersion.has_value());

    // Test 4: List currently existing keys
    auto currentKeys=store.keys();

    assert(currentKeys.size()==3);
    assert(currentKeys[0]=="apple");
    assert(currentKeys[1]=="name");
    assert(currentKeys[2]=="zebra");

    // Test 5: Delete an existing key
    bool removedName=store.remove("name");
    assert(removedName);

    result=store.get("name");
    assert(!result.has_value());
    assert(!store.exists("name"));

    history=store.getHistory("name");

    assert(history.size()==3);
    assert(history.back().operation==Operation::Delete);

    // Test 6: getVersion returns the deletion tombstone
    auto deletedNameVersion=store.getVersion("name", history.back().id);

    assert(deletedNameVersion.has_value());
    assert(deletedNameVersion->operation==Operation::Delete);
    assert(deletedNameVersion->id==history.back().id);

    // Test 7: Delete an already deleted key
    bool removedAgain=store.remove("name");
    assert(!removedAgain);

    history=store.getHistory("name");
    assert(history.size()==3);

    // Test 8: Delete a key that never existed
    bool removedMissing=store.remove("missing");
    assert(!removedMissing);

    // Test 9: KEYS excludes deleted keys
    currentKeys=store.keys();

    assert(currentKeys.size()==2);
    assert(currentKeys[0]=="apple");
    assert(currentKeys[1]=="zebra");

    // Test 10: Re-create a deleted key
    store.put("name", "Riya");

    result=store.get("name");
    assert(result.has_value());
    assert(result->value=="Riya");
    assert(store.exists("name"));

    history=store.getHistory("name");

    assert(history.size()==4);
    assert(history[2].operation==Operation::Delete);
    assert(history[3].operation==Operation::Put);
    assert(history[3].value=="Riya");
    assert(history[3].id>history[2].id);

    // Test 11: Historical lookups
    store.put("city", "Delhi");

    auto cityTime1=std::chrono::system_clock::now();

    std::this_thread::sleep_for(
        std::chrono::milliseconds(5)
    );

    store.put("city", "Mumbai");

    auto cityTime2=std::chrono::system_clock::now();

    std::this_thread::sleep_for(
        std::chrono::milliseconds(5)
    );

    bool removedCity=store.remove("city");
    assert(removedCity);

    auto cityTime3=std::chrono::system_clock::now();

    auto oldCityVersion=store.getAtTime("city", cityTime1);
    auto newCityVersion=store.getAtTime("city", cityTime2);
    auto deletedCityVersion=store.getAtTime("city", cityTime3);

    assert(oldCityVersion.has_value());
    assert(oldCityVersion->value=="Delhi");

    assert(newCityVersion.has_value());
    assert(newCityVersion->value=="Mumbai");

    assert(!deletedCityVersion.has_value());

    // Test 12: History preserves updates and deletion
    auto cityHistory=store.getHistory("city");

    assert(cityHistory.size()==3);
    assert(cityHistory[0].value=="Delhi");
    assert(cityHistory[1].value=="Mumbai");
    assert(cityHistory[2].operation==Operation::Delete);

    auto cityDeleteVersion=store.getVersion("city", cityHistory[2].id);

    assert(cityDeleteVersion.has_value());
    assert(cityDeleteVersion->operation==Operation::Delete);

    assert(store.getHistory("missing").empty());

    std::cout<<"Basic and time-travel tests passed!\n";

    // Test 13: Replay versions from WAL records
    MemoryStore recoveredStore;

    auto recoveryTime1=std::chrono::system_clock::now();
    auto recoveryTime2=recoveryTime1+std::chrono::microseconds(10);
    auto recoveryTime3=recoveryTime2+std::chrono::microseconds(10);

    Version blueVersion(10, "blue", Operation::Put, recoveryTime1);
    Version greenVersion(11, "green", Operation::Put, recoveryTime2);
    Version deleteVersion(12, "", Operation::Delete, recoveryTime3);

    bool replayBlue=recoveredStore.replayVersion("colour", blueVersion);
    bool replayGreen=recoveredStore.replayVersion("colour", greenVersion);
    bool replayDelete=recoveredStore.replayVersion("colour", deleteVersion);

    assert(replayBlue);
    assert(replayGreen);
    assert(replayDelete);

    // Test 14: Replay preserves IDs, values, operations and timestamps
    auto recoveredHistory=recoveredStore.getHistory("colour");

    assert(recoveredHistory.size()==3);

    assert(recoveredHistory[0].id==10);
    assert(recoveredHistory[0].value=="blue");
    assert(recoveredHistory[0].operation==Operation::Put);
    assert(recoveredHistory[0].timestamp==recoveryTime1);

    assert(recoveredHistory[1].id==11);
    assert(recoveredHistory[1].value=="green");
    assert(recoveredHistory[1].operation==Operation::Put);
    assert(recoveredHistory[1].timestamp==recoveryTime2);

    assert(recoveredHistory[2].id==12);
    assert(recoveredHistory[2].operation==Operation::Delete);
    assert(recoveredHistory[2].timestamp==recoveryTime3);

    // Test 15: Replayed deletion remains effective
    assert(!recoveredStore.get("colour").has_value());
    assert(!recoveredStore.exists("colour"));
    assert(recoveredStore.keys().empty());

    // Test 16: Historical versions remain accessible after replay
    auto recoveredBlue=recoveredStore.getVersion("colour", 10);
    auto recoveredGreen=recoveredStore.getVersion("colour", 11);
    auto recoveredDelete=recoveredStore.getVersion("colour", 12);

    assert(recoveredBlue.has_value());
    assert(recoveredBlue->value=="blue");

    assert(recoveredGreen.has_value());
    assert(recoveredGreen->value=="green");

    assert(recoveredDelete.has_value());
    assert(recoveredDelete->operation==Operation::Delete);

    // Test 17: Reject duplicate global version IDs
    bool duplicateRejected=!recoveredStore.replayVersion("other", blueVersion);
    assert(duplicateRejected);
    assert(recoveredStore.getHistory("other").empty());

    // Test 18: Reject invalid and out-of-order versions
    Version zeroVersion(0, "invalid", Operation::Put, recoveryTime1);
    bool zeroRejected=!recoveredStore.replayVersion("other", zeroVersion);
    assert(zeroRejected);

    Version oldVersion(9, "out of order", Operation::Put, recoveryTime1);
    bool outOfOrderRejected=!recoveredStore.replayVersion("colour", oldVersion);
    assert(outOfOrderRejected);

    assert(recoveredStore.getHistory("colour").size()==3);

    // Test 19: Next normal write uses the next available ID
    recoveredStore.put("other", "new value");

    auto nextVersion=recoveredStore.getLatest("other");

    assert(nextVersion.has_value());
    assert(nextVersion->id==13);
    assert(nextVersion->value=="new value");

    std::cout<<"Recovery tests passed!\n";

    // Test 20: Version counter must not overflow
    MemoryStore exhaustedStore;

    Version maxVersion(
        std::numeric_limits<std::uint64_t>::max(),
        "last",
        Operation::Put,
        recoveryTime1
    );

    bool maxVersionAccepted=exhaustedStore.replayVersion("last", maxVersion);
    assert(maxVersionAccepted);

    bool overflowRejected=false;

    try{
        exhaustedStore.put("next", "value");
    }
    catch(const std::overflow_error&){
        overflowRejected=true;
    }

    assert(overflowRejected);
    assert(!exhaustedStore.exists("next"));

    std::cout<<"Version overflow tests passed!\n";

    return 0;
}