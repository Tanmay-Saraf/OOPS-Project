#pragma once

#include <vector>
#include <chrono>
#include "Version.hpp"

class VersionChain {
private:
    std::vector<Version> versions;

public:
    void addVersion(const Version& version);

    const Version* getLatest() const;

    const std::vector<Version>& getAllVersions() const;

    const Version* getAtTime(
        std::chrono::system_clock::time_point time
    ) const;
};