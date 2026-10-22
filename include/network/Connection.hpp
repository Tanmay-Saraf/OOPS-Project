#pragma once

#include "network/UniqueFd.hpp"

#include <cstdint>
#include <string>

namespace oops {

    struct Connection {
        UniqueFd fd;
        std::uint64_t generation = 0;
        std::string receive_buffer;
        std::string send_buffer;
        bool discarding_oversized_request = false;
        bool close_after_write = false;
        std::uint64_t client_id = 0;
    };

}