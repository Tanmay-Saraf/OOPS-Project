#pragma once;

#include "memory/MemoryStore.hpp";
#include "memory/MemoryEngineAdapter.hpp";

namespace oops{
    class MemoryEngine: public MemoryEngineAdapter{
        private:
            MemoryStore store;
        public:
            Response excute(const Request& request) override;
    };
}