#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <mutex>
#include "VersionChain.hpp"

class MemoryStore{
    private:
        std::unordered_map<std::string, VersionChain> records;
        uint64_t nextVersionID=1;
        std::mutex mtx;
    public:
    void addVersion(const std::string &key, const Version& version){
        records[key].addVersion(version);
    }
    const Version* getLatest(const std::string& key) const{
        auto it=records.find(key);
        if(it==records.end())return nullptr;
        return it-> second.getLatest();
    }
    void put(const std::string &key, const std::string &value){
        Version version(nextVersionID++, value, Operation::Put, std::chrono::system_clock::now());
        addVersion(key, version);
    }
    bool remove(const std::string &key){
        auto it= records.find(key);
        if(it==records.end())return false;
        const Version* latest= it->second.getLatest();
        if(latest==nullptr||latest->operation==Operation::Delete){
            return false;
        }
        Version version(nextVersionID++, "", Operation::Delete, std::chrono::system_clock::now());
        it->second.addVersion(version);
        return true;
    }
    const Version* get(const std::string& key) const{
        const Version* latest=getLatest(key);
        if(latest==nullptr)return nullptr;
        if(latest->operation== Operation::Delete){
            return nullptr;
        }
        return latest;
    }
    std:: vector<Version> getHistory(const std::string& key) const{
        auto it= records.find(key);
        if(it==records.end())return {};
        return it->second.getAllVersions();
    }
    const Version* getAtTime(const std::string &key, std::chrono::system_clock::time_point time) const{
        auto it=records.find(key);
        if(it==records.end())return nullptr;
        const Version* version=it->second.getAtTime(time);
        if(version==nullptr)return nullptr;
        if(version->operation==Operation::Delete)return nullptr;
        return version;
    }
};
