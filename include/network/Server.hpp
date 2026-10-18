
#pragma once

#include "network/UniqueFd.hpp"
#include <unordered_map>
#include <network/Connection.hpp>
#include <cstdint>
#include <sys/epoll.h>
#include "common/Request.hpp"
#include "security/SecurityEngine.hpp"

namespace oops {

    class Server {
    public:
    
        explicit Server(int port);
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        ~Server() = default;

        void run();

    private:
        int port_;
        uint64_t next_generation_ = 0;
        UniqueFd server_fd_;
        UniqueFd epoll_fd_;
        SecurityEngine security_engine_;

        void setup();
        void acceptClients();
        void handleClientEvent(const epoll_event& event);
        void handleRequest(const Request& request, int client_fd);
        void handleWriteEvent(int fd);
        void updateClientEvents(int fd, std::uint32_t flags);

        std::unordered_map<int, Connection> connections_;

    };

}
