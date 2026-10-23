#pragma once

#include "recovery_manager.hpp"

class MemoryStore;

namespace storage {
    ReplayHandler make_memory_replay_handler(MemoryStore& memory);
}