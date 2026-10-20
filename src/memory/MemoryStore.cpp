#include "memory/MemoryStore.hpp"
#include <algorithm>

void MemoryStore::addVersion(const std::string &key, const Version& version){
    records[key].addVersion(version);
}

const Version* MemoryStore::getLatest(const std::string& key) const{
    auto it=records.find(key);
    if(it==records.end())return nullptr;
    return it->second.getLatest();
}

void MemoryStore::put(const std::string &key, const std::string &value){
    Version version(nextVersionID++, value, Operation::Put, std::chrono::system_clock::now());
    addVersion(key, version);
}

bool MemoryStore::remove(const std::string& key){
    auto it=records.find(key);
    if(it==records.end())return false;

    const Version* latest=it->second.getLatest();
    if(latest==nullptr || latest->operation==Operation::Delete)return false;

    Version version(nextVersionID++, "", Operation::Delete, std::chrono::system_clock::now());
    it->second.addVersion(version);
    return true;
}

const Version* MemoryStore::get(const std::string& key) const{
    const Version* latest=getLatest(key);
    if(latest==nullptr)return nullptr;
    if(latest->operation==Operation::Delete)return nullptr;
    return latest;
}

std::vector<Version> MemoryStore::getHistory(const std::string& key) const{
    auto it=records.find(key);
    if(it==records.end())return {};
    return it->second.getAllVersions();
}

const Version* MemoryStore::getAtTime(const std::string &key, std::chrono::system_clock::time_point time) const{
    auto it=records.find(key);
    if(it==records.end())return nullptr;

    const Version* version=it->second.getAtTime(time);
    if(version==nullptr)return nullptr;
    if(version->operation==Operation::Delete)return nullptr;
    return version;
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