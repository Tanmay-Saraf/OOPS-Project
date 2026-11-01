
#pragma once

#include "network/MemoryEngineAdapter.hpp"

#include <mutex>
#include <unordered_map>
#include <string>

namespace oops {

class TemporaryMemoryEngine final : public MemoryEngineAdapter {
public:
    Response execute(const Request& request) override;

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::string> records_;
};

}
