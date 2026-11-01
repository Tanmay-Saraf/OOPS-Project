#pragma once

#include "common/Response.hpp"
#include "common/Request.hpp"
#include <cstdint>

namespace oops {

struct CompletedResponse {
    int client_fd;
    std::uint64_t generation;
    std::uint64_t client_id;
    bool request_finished = false;
    Response response;
    Command command;
};

}