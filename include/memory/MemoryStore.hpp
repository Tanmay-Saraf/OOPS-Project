#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>
#include "VersionChain.hpp"

class MemoryStore{
    private:
        std::unordered_map<std::string, VersionChain> records;
        uint64_t nextVersionID=1;
        bool versionIDsExhausted=false;
        std::mutex mtx;
        void addVersion(const std::string &key, const Version& version);
        uint64_t allocateVersionID();

    public:
        std::optional<Version> getLatest(const std::string& key) const;
        void put(const std::string &key, const std::string &value);
        bool remove(const std::string& key);
        std::optional<Version> get(const std::string& key) const;
        std::vector<Version> getHistory(const std::string& key) const;
        std:: optional<Version> getAtTime(const std::string &key, std::chrono::system_clock::time_point time) const;
        bool exists(const std::string &key) const;
        std::vector<std::string> keys() const;
        std::optional<Version> getVersion(const std::string& key, uint64_t versionID) const;
        bool replayVersion(const std::string &key, const Version& version);
};