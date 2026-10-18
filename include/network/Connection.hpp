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
    };

}