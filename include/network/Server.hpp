
#pragma once

#include "network/UniqueFd.hpp"
#include "common/Request.hpp"
#include "network/RequestGuard.hpp"
#include "network/CompletedResponse.hpp"
#include "network/ThreadSafeQueue.hpp"
#include "network/Connection.hpp"
#include "network/ClientRequestState.hpp"
#include "network/MemoryEngineAdapter.hpp"

#include <unordered_map>
#include <cstdint>
#include <memory>
#include <atomic>

#include <sys/epoll.h>

namespace oops {

    class ThreadPool;

    class Server {
    public:
    
        explicit Server(int port, 
            std::unique_ptr<RequestGuard> guard = std::make_unique<AllowAllGuard>(), 
            std::unique_ptr<MemoryEngineAdapter> memory = nullptr
        );

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        ~Server();

        void run();
        void requestShutdown();

    private:
        int port_;
        uint64_t next_generation_ = 0;
        std::unordered_map<int, Connection> connections_;
        std::uint64_t next_client_id_ = 0;
        std::unordered_map<std::uint64_t, std::shared_ptr<ClientRequestState>> client_states_;

        std::atomic<std::uint64_t> outstanding_requests_{0};
        std::atomic<bool> stopping_{false};

        UniqueFd server_fd_;
        UniqueFd epoll_fd_;
        UniqueFd response_event_fd_;
        std::unique_ptr<RequestGuard> guard_;
        ThreadSafeQueue<CompletedResponse> completed_responses_;
        std::unique_ptr<ThreadPool> thread_pool_;
        std::unique_ptr<MemoryEngineAdapter> memory_;

        void setup();
        void acceptClients();
        void handleClientEvent(const int fd);
        void handleRequest(const Request& request, int client_fd);
        void handleWriteEvent(int fd);
        void updateClientEvents(int fd, std::uint32_t flags);
        void publishResponse(CompletedResponse completed);   
        void dispatchNextRequest(std::uint64_t client_id);
        bool checkAllClientBuffersEmpty() const;

    };

}
