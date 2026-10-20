#include "memory/VersionChain.hpp"

void VersionChain::addVersion(const Version& version){
    versions.push_back(version);
}

const Version* VersionChain::getLatest() const{
    if(versions.empty())return nullptr;
    return &versions.back();
}

const std::vector<Version>& VersionChain::getAllVersions() const{
    return versions;
}

const Version* VersionChain::getAtTime(std::chrono::system_clock::time_point time) const{
    for(auto it=versions.rbegin();it!=versions.rend();++it){
        if(it->timestamp<=time)return &(*it);
    }
    return nullptr;
}