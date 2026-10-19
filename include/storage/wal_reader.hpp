#pragma once 

#include "wal_record.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace storage{
    enum class WalReadStatus{
        OK,
        TRUNCATED,
        CORRUPTED,
        IO_ERROR
    };

    struct WalReadResult{
        std::vector<WalRecord> records;
        WalReadStatus status = WalReadStatus::OK;
        std::optional<std::uint64_t> error_offset;
    };

    class WalReader{
        private: 
            std::string filepath;
        public:
            WalReader(const std::string& path);
            WalReadResult read_all() const;  
    };
}