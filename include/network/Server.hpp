
#pragma once

#include "network/UniqueFd.hpp"
#include "common/Request.hpp"
#include "security/SecurityEngine.hpp"
#include "network/CompletedResponse.hpp"
#include "network/ThreadSafeQueue.hpp"
#include "network/Connection.hpp"

#include <unordered_map>
#include <cstdint>
#include <memory>

#include <sys/epoll.h>

namespace oops {

    class ThreadPool;

    class Server {
    public:
    
        explicit Server(int port);
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        ~Server();

        void run();

    private:
        int port_;
        uint64_t next_generation_ = 0;
        std::unordered_map<int, Connection> connections_;

        UniqueFd server_fd_;
        UniqueFd epoll_fd_;
        UniqueFd response_event_fd_;
        SecurityEngine security_engine_;
        ThreadSafeQueue<CompletedResponse> completed_responses_;
        std::unique_ptr<ThreadPool> thread_pool_;

        void setup();
        void acceptClients();
        void handleClientEvent(const int fd);
        void handleRequest(const Request& request, int client_fd);
        void handleWriteEvent(int fd);
        void updateClientEvents(int fd, std::uint32_t flags);
        void publishResponse(CompletedResponse completed);   

    };

}
