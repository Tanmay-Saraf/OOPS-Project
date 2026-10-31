#pragma once

#include "recovery_manager.hpp"

namespace oops{
    class MemoryStore;
}
namespace storage {
    ReplayHandler make_memory_replay_handler(oops::MemoryStore& memory);
}