#pragma once

#include <cstdint>
#include <string>

namespace storage{
    enum class RecordType : std::uint8_t {
        PUT,
        DELETE
    };

    struct WalRecord{
        RecordType type;
        std::uint64_t sequence_number;
        std::uint64_t timestamp;
        std::string key;
        std::string value;
    };
}