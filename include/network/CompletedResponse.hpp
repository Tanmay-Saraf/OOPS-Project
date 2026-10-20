#pragma once

#include "common/Response.hpp"
#include <cstdint>

namespace oops {

struct CompletedResponse {
    int client_fd;
    std::uint64_t generation;
    Response response;
};

}