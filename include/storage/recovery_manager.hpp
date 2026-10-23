#pragma once

#include "wal_reader.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace storage {

struct RecoveryReport {
    WalReadStatus status = WalReadStatus::OK;
    std::size_t records_restored = 0;
    std::optional<std::uint64_t> error_offset;
    bool replay_failed = false;

    bool fully_recovered() const noexcept {
        return status == WalReadStatus::OK && !replay_failed;
    }
};

using ReplayHandler = std::function<bool(const WalRecord&)>;

class RecoveryManager {
    public:
        RecoveryReport recover(const std::string& wal_path,const ReplayHandler& replay,std::optional<std::uint64_t> target_timestamp_us = std::nullopt) const;
};

}