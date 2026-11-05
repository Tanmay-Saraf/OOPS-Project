#pragma once

#include "common/Request.hpp"
#include "common/User.hpp"

#include <cstdint>
#include <queue>

namespace oops {

    struct ClientRequestState {
        std::uint64_t client_id = 0;
        std::queue<Request> pending_requests;

        bool request_processing = false;
        bool quit_requested = false;

        bool authenticated = false;
        UserRole role = UserRole::User;
        std::string username;

    };

}