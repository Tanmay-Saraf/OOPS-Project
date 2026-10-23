#include "memory_replay_adapter.hpp"

#include "memory/MemoryStore.hpp"
#include "memory/Version.hpp"

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>

namespace storage{

namespace{

std::optional<std::chrono::system_clock::time_point> convert_timestamp(std::uint64_t timestamp_us){
    using Clock = std::chrono::system_clock;
    using Microseconds = std::chrono::microseconds;

    if(timestamp_us >static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())){
        return std::nullopt;
    }

    const auto max_clock_us =std::chrono::duration_cast<Microseconds>(Clock::duration::max()).count();

    if(max_clock_us < 0 ||timestamp_us > static_cast<std::uint64_t>(max_clock_us)){
        return std::nullopt;
    }

    const Microseconds timestamp(static_cast<std::int64_t>(timestamp_us));

    return Clock::time_point(std::chrono::duration_cast<Clock::duration>(timestamp));
}

}

ReplayHandler make_memory_replay_handler(MemoryStore& memory) {
    return [&memory](const WalRecord& record) {
        Operation operation;

        switch(record.type){
            case RecordType::PUT:
                operation = Operation::Put;
                break;

            case RecordType::DELETE:
                operation = Operation::Delete;
                break;

            default:
                return false;
        }

        const auto timestamp = convert_timestamp(record.timestamp);

        if(!timestamp){
            return false;
        }

        Version version(
            record.sequence_number,
            record.value,
            operation,
            *timestamp
        );

        return memory.replayVersion(record.key, version);
    };
}

}