
#include <cassert>
#include <iostream>
#include <chrono>
#include <thread>
#include <limits>
#include <stdexcept>
#include <atomic>
#include <vector>
#include <algorithm>
#include "memory/MemoryEngine.hpp"

using namespace oops;

int main(){

    // ==========================================
    // MEMORY STORE TESTS
    // ==========================================

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

    assert(secondVersion.has_value());
    assert(secondVersion->value=="Alex");

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

    // Test 6: getVersion returns deletion tombstone
    auto deletedNameVersion=store.getVersion("name", history.back().id);

    assert(deletedNameVersion.has_value());
    assert(deletedNameVersion->operation==Operation::Delete);

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


    // ==========================================
    // RECOVERY TESTS
    // ==========================================

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

    // Test 13: Replay preserves original records
    auto recoveredHistory=recoveredStore.getHistory("colour");

    assert(recoveredHistory.size()==3);

    assert(recoveredHistory[0].id==10);
    assert(recoveredHistory[0].value=="blue");
    assert(recoveredHistory[0].timestamp==recoveryTime1);

    assert(recoveredHistory[1].id==11);
    assert(recoveredHistory[1].value=="green");
    assert(recoveredHistory[1].timestamp==recoveryTime2);

    assert(recoveredHistory[2].id==12);
    assert(recoveredHistory[2].operation==Operation::Delete);
    assert(recoveredHistory[2].timestamp==recoveryTime3);

    // Test 14: Replayed deletion remains effective
    assert(!recoveredStore.get("colour").has_value());
    assert(!recoveredStore.exists("colour"));
    assert(recoveredStore.keys().empty());

    // Test 15: Historical versions remain accessible
    auto recoveredBlue=recoveredStore.getVersion("colour", 10);
    auto recoveredGreen=recoveredStore.getVersion("colour", 11);
    auto recoveredDelete=recoveredStore.getVersion("colour", 12);

    assert(recoveredBlue.has_value());
    assert(recoveredBlue->value=="blue");

    assert(recoveredGreen.has_value());
    assert(recoveredGreen->value=="green");

    assert(recoveredDelete.has_value());
    assert(recoveredDelete->operation==Operation::Delete);

    // Test 16: Reject duplicate IDs
    bool duplicateRejected=!recoveredStore.replayVersion("other", blueVersion);
    assert(duplicateRejected);

    // Test 17: Reject invalid and out-of-order versions
    Version zeroVersion(0, "invalid", Operation::Put, recoveryTime1);
    assert(!recoveredStore.replayVersion("other", zeroVersion));

    Version oldVersion(9, "old", Operation::Put, recoveryTime1);
    assert(!recoveredStore.replayVersion("colour", oldVersion));

    // Test 18: Next write uses the next available ID
    recoveredStore.put("other", "new value");

    auto nextVersion=recoveredStore.getLatest("other");

    assert(nextVersion.has_value());
    assert(nextVersion->id==13);

    std::cout<<"Recovery tests passed!\n";


    // ==========================================
    // VERSION OVERFLOW TESTS
    // ==========================================

    MemoryStore exhaustedStore;

    Version maxVersion(
        std::numeric_limits<uint64_t>::max(),
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


    // ==========================================
    // CONCURRENCY TESTS
    // ==========================================

    MemoryStore concurrentStore;

    const int threadCount=4;
    const int writesPerThread=500;

    std::atomic<bool> writingDone=false;

    std::vector<std::thread> readers;
    std::vector<std::thread> writers;

    // Test 19: Concurrent readers
    for(int i=0;i<3;i++){
        readers.emplace_back([&concurrentStore,&writingDone](){
            while(!writingDone.load()){
                auto currentKeys=concurrentStore.keys();

                for(const auto& key:currentKeys){
                    auto current=concurrentStore.get(key);
                    auto latest=concurrentStore.getLatest(key);
                    auto historyCopy=concurrentStore.getHistory(key);
                    bool present=concurrentStore.exists(key);

                    auto currentTime=std::chrono::system_clock::now();
                    auto atTime=concurrentStore.getAtTime(key, currentTime);

                    if(!historyCopy.empty()){
                        auto historical=concurrentStore.getVersion(key, historyCopy.front().id);
                    }
                }
            }
        });
    }

    // Test 20: Concurrent writers
    for(int i=0;i<threadCount;i++){
        writers.emplace_back([&concurrentStore,i](){
            for(int j=0;j<writesPerThread;j++){
                concurrentStore.put(
                    "shared",
                    "thread_"+std::to_string(i)+"_"+std::to_string(j)
                );
            }
        });
    }

    for(auto& writer:writers){
        writer.join();
    }

    writingDone.store(true);

    for(auto& reader:readers){
        reader.join();
    }

    // Test 21: No writes were lost
    auto concurrentHistory=concurrentStore.getHistory("shared");

    assert(concurrentHistory.size()==threadCount*writesPerThread);

    // Test 22: Version IDs are unique and sequential
    std::vector<uint64_t> concurrentIDs;

    for(const auto& version:concurrentHistory){
        concurrentIDs.push_back(version.id);
    }

    std::sort(concurrentIDs.begin(),concurrentIDs.end());

    for(size_t i=0;i<concurrentIDs.size();i++){
        assert(concurrentIDs[i]==i+1);
    }

    // Test 23: Final state is valid
    auto concurrentLatest=concurrentStore.get("shared");

    assert(concurrentLatest.has_value());
    assert(concurrentLatest->id==threadCount*writesPerThread);
    assert(concurrentStore.exists("shared"));

    std::cout<<"Concurrency tests passed!\n";


    // ==========================================
    // MEMORY ENGINE API TESTS
    // ==========================================

    MemoryEngine engine;

    Request request;
    Response response;

    // Test 24: SET command
    request.command=Command::Set;
    request.key="name";
    request.value="Garvit";

    response=engine.execute(request);

    assert(response.status==ResponseStatus::Success);

    // Test 25: GET command
    request.command=Command::Get;

    response=engine.execute(request);

    assert(response.status==ResponseStatus::Success);
    assert(response.value=="Garvit");

    // Test 26: GET missing key
    request.key="missing";

    response=engine.execute(request);

    assert(response.status==ResponseStatus::NotFound);

    // Test 27: EXISTS command
    request.command=Command::Exists;
    request.key="name";

    response=engine.execute(request);

    assert(response.status==ResponseStatus::Success);
    assert(response.value=="true");

    request.key="missing";

    response=engine.execute(request);

    assert(response.status==ResponseStatus::Success);
    assert(response.value=="false");

    // Test 28: DELETE command
    request.command=Command::Delete;
    request.key="name";

    response=engine.execute(request);

    assert(response.status==ResponseStatus::Success);

    // Test 29: GET after deletion
    request.command=Command::Get;

    response=engine.execute(request);

    assert(response.status==ResponseStatus::NotFound);

    // Test 30: DELETE missing key
    request.command=Command::Delete;
    request.key="missing";

    response=engine.execute(request);

    assert(response.status==ResponseStatus::NotFound);

    // Test 31: KEYS command
    request.command=Command::Set;
    request.key="zebra";
    request.value="animal";

    response=engine.execute(request);
    assert(response.status==ResponseStatus::Success);

    request.key="apple";
    request.value="fruit";

    response=engine.execute(request);
    assert(response.status==ResponseStatus::Success);

    request.command=Command::Keys;

    response=engine.execute(request);

    assert(response.status==ResponseStatus::Success);
    assert(response.value=="apple;zebra");

    // Test 32: HISTORY command
    request.command=Command::History;
    request.key="apple";

    response=engine.execute(request);

    assert(response.status==ResponseStatus::Success);
    assert(response.value.find("|PUT|")!=std::string::npos);
    assert(response.value.find("fruit")!=std::string::npos);

    // Test 33: HISTORY of missing key
    request.key="missing";

    response=engine.execute(request);

    assert(response.status==ResponseStatus::NotFound);

    // Test 34: GET_VERSION command
    request.command=Command::Set;
    request.key="colour";
    request.value="blue";

    response=engine.execute(request);
    assert(response.status==ResponseStatus::Success);

    request.value="green";

    response=engine.execute(request);
    assert(response.status==ResponseStatus::Success);

    request.command=Command::GetVersion;
    request.version=5;

    response=engine.execute(request);

    assert(response.status==ResponseStatus::Success);
    assert(response.value.find("blue")!=std::string::npos);

    // Test 35: GET_VERSION missing version
    request.version=999999;

    response=engine.execute(request);

    assert(response.status==ResponseStatus::NotFound);

    // Test 36: GET_VERSION tombstone
    request.command=Command::Delete;
    request.key="colour";

    response=engine.execute(request);
    assert(response.status==ResponseStatus::Success);

    request.command=Command::GetVersion;
    request.version=7;

    response=engine.execute(request);

    assert(response.status==ResponseStatus::NotFound);
    assert(response.value.find("DELETE")!=std::string::npos);

    // Test 37: Invalid SET request
    request.command=Command::Set;
    request.key="";
    request.value="value";

    response=engine.execute(request);

    assert(response.status==ResponseStatus::InvalidRequest);

    // Test 38: Invalid GET request
    request.command=Command::Get;
    request.key="";

    response=engine.execute(request);

    assert(response.status==ResponseStatus::InvalidRequest);

    // Test 39: Invalid GET_VERSION request
    request.command=Command::GetVersion;
    request.key="colour";
    request.version=0;

    response=engine.execute(request);

    assert(response.status==ResponseStatus::InvalidRequest);

    // Test 40: Invalid command
    request.command=Command::Invalid;

    response=engine.execute(request);

    assert(response.status==ResponseStatus::InvalidRequest);

    std::cout<<"Memory Engine API tests passed!\n";

    return 0;
}
