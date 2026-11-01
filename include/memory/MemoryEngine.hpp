#pragma once

#include "memory/MemoryStore.hpp"
#include "common/Request.hpp"
#include "common/Response.hpp"
#include "network/MemoryEngineAdapter.hpp"
#include <mutex>

namespace oops{
    class MemoryEngine: public MemoryEngineAdapter{
        private:
            MemoryStore store;
            WalAppendHandler walAppendHandler;
            std::mutex walHandlerMtx;
        public:
            void setWalAppendHandler(WalAppendHandler handler);
            Response execute(const Request& request) override;
            bool replayVersion(const std::string &key, const Version& version);
            void clear();
    };
}