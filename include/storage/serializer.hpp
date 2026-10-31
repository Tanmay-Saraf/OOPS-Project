#pragma once 

#include <cstdint>
#include <vector>
#include <optional>
#include "wal_record.hpp"

namespace storage{
    std::vector<std::uint8_t> serialize(const storage::WalRecord& record);

    std::optional<storage::WalRecord> deserialize(const std::vector<std::uint8_t>& bytes);
}