#pragma once

#include "recovery_manager.hpp"
#include "memory/MemoryStore.hpp"

namespace oops {
    class MemoryEngine;
}

namespace storage {
    class WalWriter;

    // Existing adapter for code that owns a MemoryStore directly.
    ReplayHandler make_memory_replay_handler(oops::MemoryStore& memory);

    // Adapters for the public MemoryEngine API used by the server.
    ReplayHandler make_memory_replay_handler(oops::MemoryEngine& memory);
    ClearHandler make_memory_clear_handler(oops::MemoryEngine& memory);
    oops::WalAppendHandler make_wal_append_handler(WalWriter& writer);
}
