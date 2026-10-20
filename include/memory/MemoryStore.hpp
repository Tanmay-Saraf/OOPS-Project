#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "VersionChain.hpp"

class MemoryStore{
    private:
        std::unordered_map<std::string, VersionChain> records;
        uint64_t nextVersionID=1;
        std::mutex mtx;

    public:
    void addVersion(const std::string &key, const Version& version);
    const Version* getLatest(const std::string& key) const;
    void put(const std::string &key, const std::string &value);
    bool remove(const std::string& key);
    const Version* get(const std::string& key) const;
    std::vector<Version> getHistory(const std::string& key) const;
    const Version* getAtTime(const std::string &key, std::chrono::system_clock::time_point time) const;
    bool exists(const std::string &key) const;
    std::vector<std::string> keys() const;
};