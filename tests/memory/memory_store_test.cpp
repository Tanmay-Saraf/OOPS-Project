
#include <cassert>
#include <iostream>
#include <chrono>
#include <thread>
#include <limits>
#include <stdexcept>
#include <atomic>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <string>
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

                    // These reads are intentionally concurrent snapshots; their values
                    // are not compared because a writer may update the key between calls.
                    (void)current;
                    (void)latest;
                    (void)present;
                    (void)atTime;

                    if(!historyCopy.empty()){
                        auto historical=concurrentStore.getVersion(key, historyCopy.front().id);
                        (void)historical;
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



    // ==========================================
    // WAL CALLBACK TESTS
    // ==========================================

    // Test 41: PUT callback receives the generated Version before memory commit
    MemoryStore callbackStore;
    bool putCallbackCalled=false;
    bool putWasUncommitted=false;
    std::uint64_t callbackVersionID=0;
    Operation callbackOperation=Operation::Delete;
    std::string callbackKey;
    std::string callbackValue;

    callbackStore.put("wal_key", "wal_value", [&](const std::string& key, const Version& version){
        putCallbackCalled=true;
        callbackKey=key;
        callbackVersionID=version.id;
        callbackOperation=version.operation;
        callbackValue=version.value;
        putWasUncommitted=!callbackStore.get(key).has_value();
    });

    assert(putCallbackCalled);
    assert(putWasUncommitted);
    assert(callbackKey=="wal_key");
    assert(callbackVersionID==1);
    assert(callbackOperation==Operation::Put);
    assert(callbackValue=="wal_value");
    assert(callbackStore.get("wal_key")->value=="wal_value");

    // Test 42: Failed PUT callback prevents the memory mutation
    MemoryStore failedPutStore;
    bool putExceptionCaught=false;

    try{
        failedPutStore.put("failed", "value", [](const std::string&, const Version&){
            throw std::runtime_error("WAL append failed");
        });
    }
    catch(const std::runtime_error&){
        putExceptionCaught=true;
    }

    assert(putExceptionCaught);
    assert(!failedPutStore.get("failed").has_value());
    assert(failedPutStore.getHistory("failed").empty());

    // A failed callback consumes its reserved ID, but does not add a Version.
    failedPutStore.put("after_failure", "value");
    auto afterFailedPut=failedPutStore.getLatest("after_failure");
    assert(afterFailedPut.has_value());
    assert(afterFailedPut->id==2);

    // Test 43: DELETE callback runs before the tombstone is committed
    MemoryStore callbackDeleteStore;
    callbackDeleteStore.put("delete_me", "original");

    bool deleteCallbackCalled=false;
    bool deleteSawOriginalValue=false;
    bool callbackReceivedTombstone=false;

    bool callbackDeleteResult=callbackDeleteStore.remove(
        "delete_me",
        [&](const std::string& key, const Version& version){
            deleteCallbackCalled=true;
            auto beforeDelete=callbackDeleteStore.get(key);
            deleteSawOriginalValue=beforeDelete.has_value() && beforeDelete->value=="original";
            callbackReceivedTombstone=version.operation==Operation::Delete && version.value.empty();
        }
    );

    assert(callbackDeleteResult);
    assert(deleteCallbackCalled);
    assert(deleteSawOriginalValue);
    assert(callbackReceivedTombstone);
    assert(!callbackDeleteStore.get("delete_me").has_value());

    // Test 44: Failed DELETE callback leaves the key unchanged
    MemoryStore failedDeleteStore;
    failedDeleteStore.put("keep", "important");
    bool deleteExceptionCaught=false;

    try{
        failedDeleteStore.remove("keep", [](const std::string&, const Version&){
            throw std::runtime_error("WAL append failed");
        });
    }
    catch(const std::runtime_error&){
        deleteExceptionCaught=true;
    }

    assert(deleteExceptionCaught);
    auto stillPresent=failedDeleteStore.get("keep");
    assert(stillPresent.has_value());
    assert(stillPresent->value=="important");
    assert(failedDeleteStore.getHistory("keep").size()==1);

    // Test 45: Invalid DELETE operations do not invoke the callback
    MemoryStore invalidDeleteStore;
    int invalidDeleteCallbackCalls=0;
    auto countDeleteCallback=[&](const std::string&, const Version&){
        invalidDeleteCallbackCalls++;
    };

    assert(!invalidDeleteStore.remove("missing", countDeleteCallback));
    assert(invalidDeleteCallbackCalls==0);

    invalidDeleteStore.put("once", "value");
    assert(invalidDeleteStore.remove("once", countDeleteCallback));
    assert(invalidDeleteCallbackCalls==1);
    assert(!invalidDeleteStore.remove("once", countDeleteCallback));
    assert(invalidDeleteCallbackCalls==1);

    // Test 46: Concurrent callback writes preserve callback and memory order
    MemoryStore walConcurrentStore;
    const int walThreadCount=4;
    const int walWritesPerThread=100;
    std::vector<std::uint64_t> persistedIDs;
    std::mutex persistedIDsMtx;
    std::vector<std::thread> walWriters;

    for(int i=0;i<walThreadCount;i++){
        walWriters.emplace_back([&walConcurrentStore,&persistedIDs,&persistedIDsMtx,i](){
            for(int j=0;j<walWritesPerThread;j++){
                walConcurrentStore.put(
                    "shared",
                    "value_"+std::to_string(i)+"_"+std::to_string(j),
                    [&](const std::string& key, const Version& version){
                        assert(key=="shared");
                        std::lock_guard<std::mutex> lock(persistedIDsMtx);
                        persistedIDs.push_back(version.id);
                    }
                );
            }
        });
    }

    for(auto& writer:walWriters){
        writer.join();
    }

    auto walConcurrentHistory=walConcurrentStore.getHistory("shared");
    assert(walConcurrentHistory.size()==walThreadCount*walWritesPerThread);
    assert(persistedIDs.size()==walConcurrentHistory.size());

    for(size_t i=0;i<persistedIDs.size();i++){
        assert(persistedIDs[i]==i+1);
        assert(walConcurrentHistory[i].id==persistedIDs[i]);
    }

    std::cout<<"WAL callback tests passed!\n";


    // ==========================================
    // MEMORY ENGINE WAL / RECOVERY API TESTS
    // ==========================================

    // Test 47: MemoryEngine passes SET versions to its registered callback
    MemoryEngine callbackEngine;
    int engineCallbackCalls=0;
    std::string engineCallbackKey;
    std::uint64_t engineCallbackVersionID=0;
    Operation engineCallbackOperation=Operation::Delete;
    bool engineCallbackSawUncommittedValue=false;

    callbackEngine.setWalAppendHandler([&](const std::string& key, const Version& version){
        engineCallbackCalls++;
        engineCallbackKey=key;
        engineCallbackVersionID=version.id;
        engineCallbackOperation=version.operation;

        Request readDuringCallback;
        readDuringCallback.command=Command::Get;
        readDuringCallback.key=key;
        Response beforeCommit=callbackEngine.execute(readDuringCallback);
        engineCallbackSawUncommittedValue=beforeCommit.status==ResponseStatus::NotFound;
    });

    Request walRequest;
    walRequest.command=Command::Set;
    walRequest.key="engine_wal_key";
    walRequest.value="engine_wal_value";
    Response walResponse=callbackEngine.execute(walRequest);

    assert(walResponse.status==ResponseStatus::Success);
    assert(engineCallbackCalls==1);
    assert(engineCallbackKey=="engine_wal_key");
    assert(engineCallbackVersionID==1);
    assert(engineCallbackOperation==Operation::Put);
    assert(engineCallbackSawUncommittedValue);

    // Test 48: MemoryEngine reports a failed WAL callback and does not commit SET
    callbackEngine.setWalAppendHandler([](const std::string&, const Version&){
        throw std::runtime_error("WAL append failed");
    });

    walRequest.key="engine_failed_key";
    walRequest.value="must_not_appear";
    walResponse=callbackEngine.execute(walRequest);

    assert(walResponse.status==ResponseStatus::InternalError);

    walRequest.command=Command::Get;
    walResponse=callbackEngine.execute(walRequest);
    assert(walResponse.status==ResponseStatus::NotFound);

    // Test 49: MemoryEngine does not call WAL for an invalid DELETE
    int invalidEngineDeleteCalls=0;
    callbackEngine.setWalAppendHandler([&](const std::string&, const Version&){
        invalidEngineDeleteCalls++;
    });

    walRequest.command=Command::Delete;
    walRequest.key="does_not_exist";
    walResponse=callbackEngine.execute(walRequest);
    assert(walResponse.status==ResponseStatus::NotFound);
    assert(invalidEngineDeleteCalls==0);

    // Test 50: MemoryEngine passes DELETE tombstones to the callback before commit
    walRequest.command=Command::Set;
    walRequest.key="engine_delete_key";
    walRequest.value="still_here_during_callback";
    walResponse=callbackEngine.execute(walRequest);
    assert(walResponse.status==ResponseStatus::Success);

    bool engineDeleteSawOldValue=false;
    bool engineDeleteCallbackReceivedTombstone=false;
    callbackEngine.setWalAppendHandler([&](const std::string& key, const Version& version){
        engineDeleteCallbackReceivedTombstone=version.operation==Operation::Delete && version.value.empty();
        Request readDuringCallback;
        readDuringCallback.command=Command::Get;
        readDuringCallback.key=key;
        Response beforeCommit=callbackEngine.execute(readDuringCallback);
        engineDeleteSawOldValue=beforeCommit.status==ResponseStatus::Success &&
                                beforeCommit.value=="still_here_during_callback";
    });

    walRequest.command=Command::Delete;
    walRequest.key="engine_delete_key";
    walResponse=callbackEngine.execute(walRequest);
    assert(walResponse.status==ResponseStatus::Success);
    assert(engineDeleteSawOldValue);
    assert(engineDeleteCallbackReceivedTombstone);

    // Test 51: Failed MemoryEngine DELETE callback preserves the key
    walRequest.command=Command::Set;
    walRequest.key="engine_protected_key";
    walRequest.value="protected";
    callbackEngine.setWalAppendHandler([](const std::string&, const Version&){});
    walResponse=callbackEngine.execute(walRequest);
    assert(walResponse.status==ResponseStatus::Success);

    callbackEngine.setWalAppendHandler([](const std::string&, const Version&){
        throw std::runtime_error("WAL append failed");
    });

    walRequest.command=Command::Delete;
    walRequest.key="engine_protected_key";
    walResponse=callbackEngine.execute(walRequest);
    assert(walResponse.status==ResponseStatus::InternalError);

    walRequest.command=Command::Get;
    walResponse=callbackEngine.execute(walRequest);
    assert(walResponse.status==ResponseStatus::Success);
    assert(walResponse.value=="protected");

    // Test 52: MemoryEngine replay forwarding and clear forwarding
    MemoryEngine recoveryEngine;
    Version forwardedVersion(
        42,
        "recovered_value",
        Operation::Put,
        std::chrono::system_clock::now()
    );

    assert(recoveryEngine.replayVersion("recovered_key", forwardedVersion));
    assert(!recoveryEngine.replayVersion("another_key", forwardedVersion));

    Request recoveryRequest;
    recoveryRequest.command=Command::Get;
    recoveryRequest.key="recovered_key";
    Response recoveryResponse=recoveryEngine.execute(recoveryRequest);
    assert(recoveryResponse.status==ResponseStatus::Success);
    assert(recoveryResponse.value=="recovered_value");

    // Replay must also advance the next version ID through MemoryEngine.
    recoveryRequest.command=Command::Set;
    recoveryRequest.key="written_after_replay";
    recoveryRequest.value="new_value";
    recoveryResponse=recoveryEngine.execute(recoveryRequest);
    assert(recoveryResponse.status==ResponseStatus::Success);

    recoveryRequest.command=Command::History;
    recoveryResponse=recoveryEngine.execute(recoveryRequest);
    assert(recoveryResponse.status==ResponseStatus::Success);
    assert(recoveryResponse.value.find("43|PUT|")==0);

    recoveryEngine.clear();
    recoveryResponse=recoveryEngine.execute(recoveryRequest);
    assert(recoveryResponse.status==ResponseStatus::NotFound);

    // clear() must reset version numbering as well as remove records.
    recoveryRequest.command=Command::Set;
    recoveryRequest.key="after_clear";
    recoveryRequest.value="fresh_database";
    recoveryResponse=recoveryEngine.execute(recoveryRequest);
    assert(recoveryResponse.status==ResponseStatus::Success);

    recoveryRequest.command=Command::History;
    recoveryResponse=recoveryEngine.execute(recoveryRequest);
    assert(recoveryResponse.status==ResponseStatus::Success);
    assert(recoveryResponse.value.find("1|PUT|")==0);

    std::cout<<"MemoryEngine WAL and recovery API tests passed!\n";

    return 0;
}
