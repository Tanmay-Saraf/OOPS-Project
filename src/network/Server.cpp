//File Includes
#include "network/Server.hpp"
#include "network/RequestParser.hpp"
#include "network/ResponseSerializer.hpp"
#include "common/Response.hpp"
#include "security/SecurityEngine.hpp"
#include "network/CompletedResponse.hpp"
#include "network/ThreadSafeQueue.hpp"
#include "network/ThreadPool.hpp"

//C++ Header Includes
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <utility>
#include <iostream>
#include <algorithm>

//System Calls Includes
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/eventfd.h>

namespace oops{

    Server::Server(int port) : port_(port) {
        if (port_ < 1 || port_ > 65535) {
            throw std::invalid_argument("Invalid port number");
        }
        thread_pool_ = std::make_unique<ThreadPool>(4);
        setup();
    }

    Server::~Server() = default;

    void Server::setup(){

        //Server Initialization
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if(fd == -1){
            throw std::runtime_error("Failed to create socket");
        }

        server_fd_ = UniqueFd(fd);

        struct sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_port = htons(port_);
        address.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(server_fd_.get(), (sockaddr*)&address, sizeof(address)) == -1){
            throw std::runtime_error("Failed to bind server to ip and port");
        }

        if(listen(server_fd_.get(), 128) == -1){
            throw std::runtime_error("Failed to start listening");
        }

        //Make it nonblocking
        int flags = fcntl(server_fd_.get(), F_GETFL, 0);
        if(flags == -1){
            throw std::runtime_error("Failed to get flags");
        }

        if(fcntl(server_fd_.get(), F_SETFL, flags | O_NONBLOCK) == -1){
            throw std::runtime_error("Failed to set non blocking");
        }

        //Epoll Setup
        int epfd =  epoll_create1(EPOLL_CLOEXEC);
        if(epfd == -1){
            throw std::runtime_error("Failed to create epoll");
        }

        epoll_fd_ = UniqueFd(epfd);

        response_event_fd_ = UniqueFd(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC));

        if (response_event_fd_.get() == -1) {
            throw std::runtime_error("eventfd creation failed");
        }

        epoll_event server_event{};
        server_event.data.fd = server_fd_.get();
        server_event.events = EPOLLIN;

        if(epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, server_fd_.get(), &server_event) == -1){
            throw std::runtime_error("Failed to register listening socket with epoll");
        }

        epoll_event response_event{};
        response_event.data.fd = response_event_fd_.get();
        response_event.events = EPOLLIN;

        if(epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, response_event_fd_.get(), &response_event) == -1) {
            throw std::runtime_error("Failed to register response fd with epoll");
        }

    }

    void Server::acceptClients() {

        while(true){

            int client_fd = accept(server_fd_.get(), nullptr, nullptr);

            if(client_fd == -1){
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    break;
                }else {
                    throw std::runtime_error("Failed to accept client connection");
                }
            }

            UniqueFd client(client_fd);
            int client_flags = fcntl(client.get(), F_GETFL, 0);
            if(client_flags == -1){
                throw std::runtime_error("Could not retrieve client flags");
            }

            if(fcntl(client.get(), F_SETFL, client_flags | O_NONBLOCK) == -1){
                throw std::runtime_error("Could not make client non blocking");
            }

            Connection conn;
            conn.fd = std::move(client);
            conn.generation = next_generation_++;

            epoll_event client_event{};
            client_event.data.u64 = (static_cast<std::uint64_t>(conn.generation) << 32) | static_cast<std::uint32_t>(conn.fd.get());
            client_event.events = EPOLLIN;

            if(epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, conn.fd.get(), &client_event) == -1){
                throw std::runtime_error("Failed to add client to epoll");
            }

            connections_.emplace(client_fd, std::move(conn));
        }

    }

    void Server::handleClientEvent(const int fd){

        constexpr std::size_t MAX_REQUEST_SIZE = 4096;
        constexpr int buffer_size = 1024;
        char buffer[buffer_size];
        int bytes_recvd;

        auto it = connections_.find(fd);

        if(it == connections_.end()) return;

        Connection& conn = it->second;

        while(true){
            bytes_recvd = recv(conn.fd.get(), buffer, buffer_size, 0);

            if(bytes_recvd > 0){

                if(conn.discarding_oversized_request) {
                    
                    auto newline = std::find(buffer, buffer + bytes_recvd, '\n');

                    if(newline == buffer+bytes_recvd) {
                        continue;
                    }else {
                        char* remaining = newline + 1;
                        std::size_t remaining_size = (buffer + bytes_recvd) - remaining;
                        conn.receive_buffer.append(remaining, remaining_size);
                        conn.discarding_oversized_request = false;
                    }

                } else conn.receive_buffer.append(buffer, bytes_recvd);

                size_t pos;
                std::string &curr_receive_buffer = conn.receive_buffer;
                while((pos = curr_receive_buffer.find('\n')) != std::string::npos){

                    std::string cmd = curr_receive_buffer.substr(0, pos);
                    curr_receive_buffer.erase(0, pos+1);
                    if (!cmd.empty() && cmd.back() == '\r') {
                        cmd.pop_back();
                    }

                    Request request = parseRequest(cmd);

                    if(request.command == Command::Invalid) {

                        Response response;
                        response.status = ResponseStatus::InvalidRequest;
                        response.message = "Invalid Request";
                        conn.send_buffer.append(serializeResponse(response));
                        updateClientEvents(conn.fd.get(), EPOLLIN | EPOLLOUT);

                    }
                    else{
                        std::cout << "Received valid request: " << cmd << '\n';
                        handleRequest(request, conn.fd.get());
                    }

                }

                if(curr_receive_buffer.size() > MAX_REQUEST_SIZE) {
                    conn.discarding_oversized_request = true;
                    curr_receive_buffer.clear();

                    Response response;
                    response.status = ResponseStatus::InvalidRequest;
                    response.message = "Invalid Request";

                    conn.send_buffer.append(serializeResponse(response));
                    updateClientEvents(conn.fd.get(), EPOLLIN | EPOLLOUT);
                }

                continue;
            }

            if(bytes_recvd == 0){
                epoll_ctl(epoll_fd_.get(), EPOLL_CTL_DEL, conn.fd.get(), nullptr);
                connections_.erase(conn.fd.get());
                return;
            }

            if(bytes_recvd == -1){
                if(errno == EINTR){
                    continue;
                }
                if(errno == EAGAIN || errno == EWOULDBLOCK) break;

                int fd = conn.fd.get();
                epoll_ctl(epoll_fd_.get(), EPOLL_CTL_DEL, fd, nullptr);
                connections_.erase(fd);
                return;
            }
        }

    }

    void Server::run(){

        constexpr int MAX_EVENTS = 64;
        epoll_event events[MAX_EVENTS];

        while(true){

            int active = epoll_wait(epoll_fd_.get(), events, MAX_EVENTS, -1);

            if(active == -1 && errno == EINTR){
                continue;
            }

            if(active == -1){
                throw std::runtime_error("Epoll wait failed");
            }

            for(int i = 0; i<active; i++){

                std::uint64_t token = events[i].data.u64;

                int event_fd = static_cast<int>(token & 0xFFFFFFFFULL);
                std::uint32_t generation = static_cast<std::uint32_t>(token >> 32);


                if(event_fd == server_fd_.get()){
                    acceptClients();
                } else if(event_fd == response_event_fd_.get()) {

                    std::uint64_t notifications = 0;

                    ssize_t bytes_read = read(response_event_fd_.get(), &notifications, sizeof(notifications));
                    if (bytes_read == -1) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                            throw std::runtime_error("Failed to read response eventfd");
                        }
                    }

                    CompletedResponse completed;

                    while(completed_responses_.tryPop(completed)) {

                        auto it = connections_.find(completed.client_fd);

                        if(it == connections_.end()) continue;

                        if(completed.generation != it->second.generation) continue;

                        it->second.send_buffer.append(serializeResponse(completed.response));
                        updateClientEvents(completed.client_fd, EPOLLIN | EPOLLOUT);

                    }

                }
                else {
                    auto it = connections_.find(event_fd);
                    if(it == connections_.end()) continue;
                    Connection &conn = it->second;
                    if(conn.generation != generation) continue;
                    if(events[i].events & EPOLLIN) handleClientEvent(event_fd);
                    if(events[i].events & EPOLLOUT) handleWriteEvent(event_fd);
                }

            }

        }
    }

    void Server::handleRequest(const Request& request, int client_fd) {

        auto it = connections_.find(client_fd);

        if(it == connections_.end()) return;

        Connection& conn = it->second;

        Response response;

        if(request.command == Command::Ping) {
            response.status = ResponseStatus::Success;
            response.message = "PONG";
            conn.send_buffer.append(serializeResponse(response));
            updateClientEvents(it->first, EPOLLIN | EPOLLOUT);
            return;
        }else if(request.command == Command::Quit) {
            response.status = ResponseStatus::Success;
            response.message = "BYE";
            conn.send_buffer.append(serializeResponse(response));
            conn.close_after_write = true;
            updateClientEvents(it->first, EPOLLIN | EPOLLOUT);
            return;
        }

        const std::uint64_t generation = conn.generation;

        thread_pool_->enqueue([request, client_fd, generation, this] () {
            SecurityDecision decision = security_engine_.processRequest(request);
            Response response;
            if(!decision.allowed) {
                response.message = decision.reason;
                response.status = ResponseStatus::PermissionDenied;
                CompletedResponse completedResponse;
                completedResponse.client_fd = client_fd;
                completedResponse.generation = generation;
                completedResponse.response = response;
                publishResponse(std::move(completedResponse));
                return;
            }else {

                response.message = "Request processing not implemented";
                response.status = ResponseStatus::InternalError;
                CompletedResponse completedResponse;
                completedResponse.client_fd = client_fd;
                completedResponse.generation = generation;
                completedResponse.response = response;
                publishResponse(std::move(completedResponse));
                return;

            }

        });
    }

    void Server::handleWriteEvent(int fd) {

        auto it = connections_.find(fd);

        if (it == connections_.end()) {
            return;
        }

        Connection& conn = it->second;
        std::string &send_buffer = conn.send_buffer;
        ssize_t sent_bytes;

        while(true){

            if(send_buffer.empty()) {

                if(conn.close_after_write) {
                    epoll_ctl(epoll_fd_.get(), EPOLL_CTL_DEL, conn.fd.get(), nullptr);
                    connections_.erase(conn.fd.get());
                }

                updateClientEvents(conn.fd.get(), EPOLLIN);
                break;
            }
            sent_bytes = send(conn.fd.get(), send_buffer.data(), send_buffer.size(), MSG_NOSIGNAL);

            if(sent_bytes > 0) {
                send_buffer.erase(0, sent_bytes);
                continue;
            } else if(sent_bytes == 0) {
                epoll_ctl(epoll_fd_.get(), EPOLL_CTL_DEL, conn.fd.get(), nullptr);
                connections_.erase(conn.fd.get());
                return;
            }
            else if(sent_bytes == -1) {

                if(errno == EINTR){
                    continue;
                }
                if(errno == EAGAIN || errno == EWOULDBLOCK) break;

                int fd = conn.fd.get();
                epoll_ctl(epoll_fd_.get(), EPOLL_CTL_DEL, fd, nullptr);
                connections_.erase(fd);
                return;
            }

        }

    }

    void Server::updateClientEvents(int fd, std::uint32_t flags){

        auto it = connections_.find(fd);

        if(it == connections_.end()) return;

        Connection &conn = it->second;

        epoll_event event{};
        event.data.u64 = static_cast<uint64_t>(conn.generation)<<32 | static_cast<uint32_t>(conn.fd.get());
        event.events = flags;

        if(epoll_ctl(epoll_fd_.get(), EPOLL_CTL_MOD, fd, &event) == -1){
            throw std::runtime_error("Modifying Client Events failed");
        }

    }

    void Server::publishResponse(CompletedResponse completedResponse) {

        completed_responses_.push(std::move(completedResponse));
        const std::uint64_t notification = 1;
        ssize_t bytes_written;

        while(true) {
            bytes_written = write(response_event_fd_.get(), &notification, sizeof(notification));
            
            if(bytes_written == static_cast<ssize_t>(sizeof(notification))) break;

            if(bytes_written == -1){
                if(errno == EINTR) continue;
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    // eventfd is already readable, a notification is pending.
                    break;
                }
            } 
            
            throw std::runtime_error("Error while updating notification");

        }

    }

}