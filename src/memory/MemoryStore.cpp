#include "memory/MemoryStore.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

void MemoryStore::addVersion(const std::string &key, const Version& version){
    records[key].addVersion(version);
}

std::optional<Version> MemoryStore::getLatest(const std::string& key) const{
    auto it=records.find(key);
    if(it==records.end())return std::nullopt;
    const Version* latest=it->second.getLatest();
    if(latest==nullptr) return std::nullopt;
    return *latest;
}

uint64_t MemoryStore::allocateVersionID(){
    if(versionIDsExhausted)throw std::overflow_error("MemoryStore version IDs exhausted");
    uint64_t id=nextVersionID;
    if(nextVersionID==std::numeric_limits<uint64_t>::max()){
        versionIDsExhausted=true;
    }else{
        nextVersionID++;
    }
    return id;
}
void MemoryStore::put(const std::string &key, const std::string &value){
    Version version(allocateVersionID(), value, Operation::Put, std::chrono::system_clock::now());
    addVersion(key, version);
}

bool MemoryStore::remove(const std::string& key){
    auto it=records.find(key);
    if(it==records.end())return false;

    const Version* latest=it->second.getLatest();
    if(latest==nullptr || latest->operation==Operation::Delete)return false;

    Version version(allocateVersionID(), "", Operation::Delete, std::chrono::system_clock::now());
    it->second.addVersion(version);
    return true;
}

std::optional<Version> MemoryStore::get(const std::string& key) const{
    std::optional <Version> latest= getLatest(key);
    if(!latest.has_value()) return std::nullopt;
    if(latest->operation==Operation::Delete)return std::nullopt;
    return latest;
}

std::vector<Version> MemoryStore::getHistory(const std::string& key) const{
    auto it=records.find(key);
    if(it==records.end())return {};
    return it->second.getAllVersions();
}

std::optional<Version> MemoryStore::getAtTime(const std::string &key, std::chrono::system_clock::time_point time) const{
    auto it=records.find(key);
    if(it==records.end())return std::nullopt;

    const Version* version=it->second.getAtTime(time);
    if(version==nullptr)return std::nullopt;
    if(version->operation==Operation::Delete)return std::nullopt;
    return *version;
}

bool MemoryStore::exists(const std:: string &key) const{
    auto it=records.find(key);
    if(it==records.end())return false;
    const Version* latest=it->second.getLatest();
    if(latest==nullptr) return false;
    return latest->operation!=Operation::Delete;
}

std::vector<std::string> MemoryStore::keys() const{
    std::vector<std::string> result;
    for(const auto& entry: records){
        const Version* latest=entry.second.getLatest();
        if(latest!=nullptr && latest->operation!=Operation::Delete){
            result.push_back(entry.first);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::optional<Version> MemoryStore:: getVersion(const std::string& key, uint64_t VersionID) const{
    auto it=records.find(key);
    if(it==records.end())return std::nullopt;
    const auto& versions= it->second.getAllVersions();
    for(const auto& version: versions){
        if(version.id==VersionID){
            return version;
        }
    }
    return std::nullopt;
}
bool MemoryStore::replayVersion(const std::string &key, const Version& version){
    if(version.id==0)return false;
    for(const auto &entry:records){
        for(const auto& existing:entry.second.getAllVersions()){
            if(existing.id==version.id)return false;
        }
    }
    auto it=records.find(key);
    if(it!=records.end()){
        const Version* latest=it->second.getLatest();
        if(latest!=nullptr && latest->id>=version.id)return false;
    }
    records[key].addVersion(version);
    if(!versionIDsExhausted && version.id>=nextVersionID){
        if(version.id==std::numeric_limits<uint64_t>::max()){
            versionIDsExhausted=true;
        }else{
            nextVersionID=version.id+1;
        }
    }
    return true;
}