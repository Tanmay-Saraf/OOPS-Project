#pragma once
#include <vector>
#include <chrono>
#include "Version.hpp"

class VersionChain{
    private:
        std::vector<Version> versions;
    
    public:
        void addVersion(const Version &version){
            versions.push_back(version);
        }
        const Version* getLatest() const{
            if(versions.empty()){
                return nullptr;
            }
            return &versions.back();
        }
        const std::vector<Version>& getAllVersions() const{
            return versions;
        }
        const Version* getAtTime(std::chrono::system_clock::time_point time)const{
            for(auto it=versions.rbegin(); it!= versions.rend(); ++it){
                if(it->timestamp<=time){
                    return &(*it);
                }
            }
            return nullptr;
        }
};