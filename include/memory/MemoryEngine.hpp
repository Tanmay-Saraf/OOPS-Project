#pragma once

#include "memory/MemoryStore.hpp"
#include "common/Request.hpp"
#include "common/Response.hpp"
#include "network/MemoryEngineAdapter.hpp"

namespace oops{
    class MemoryEngine: public MemoryEngineAdapter{
        private:
            MemoryStore store;
        public:
            Response execute(const Request& request) override;
    };
}