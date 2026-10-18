#include "network/Server.hpp"
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cerrno>
#include <utility>
#include "network/RequestParser.hpp"
#include "common/Response.hpp"
#include "network/ResponseSerializer.hpp"
#include <iostream>

namespace oops{

    Server::Server(int port) : port_(port) {
        if (port_ < 1 || port_ > 65535) {
            throw std::invalid_argument("Invalid port number");
        }

        setup();
    }

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

        epoll_event event{};
        event.data.fd = server_fd_.get();
        event.events = EPOLLIN;

        if(epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, server_fd_.get(), &event) == -1){
            throw std::runtime_error("Failed to register listening socket with epoll");
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

            if(epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, client.get(), &client_event) == -1){
                throw std::runtime_error("Failed to add client to epoll");
            }

            connections_.emplace(client_fd, std::move(conn));
        }

    }

    void Server::handleClientEvent(const epoll_event& event){

        constexpr int buffer_size = 1024;
        char buffer[buffer_size];
        int bytes_recvd;

        auto it = connections_.find(event.data.fd);

        if(it == connections_.end()) return;

        Connection& conn = it->second;

        while(true){
            bytes_recvd = recv(conn.fd.get(), buffer, buffer_size, 0);

            if(bytes_recvd > 0){
                conn.receive_buffer.append(buffer, bytes_recvd);

                size_t pos;
                std::string &curr_receive_buffer = conn.receive_buffer;
                while((pos = curr_receive_buffer.find('\n')) != std::string::npos){

                    std::string cmd = curr_receive_buffer.substr(0, pos);
                    curr_receive_buffer.erase(0, pos+1);
                    if (!cmd.empty() && cmd.back() == '\r') {
                        cmd.pop_back();
                    }

                    Request request = parseRequest(cmd);

                    if(request.command == Command::Invalid) std::cout<<"Invalid\n";
                    else{
                        std::cout << "Received valid request: " << cmd << '\n';
                        handleRequest(request, conn.fd.get());
                    }

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

                int event_fd = events[i].data.fd;

                if(event_fd == server_fd_.get()){
                    acceptClients();
                }else {
                    if(events[i].events & EPOLLIN) handleClientEvent(events[i]);
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
        }


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

        epoll_event event{};
        event.data.fd = fd;
        event.events = flags;

        if(epoll_ctl(epoll_fd_.get(), EPOLL_CTL_MOD, fd, &event) == -1){
            throw std::runtime_error("Modifying Client Events failed");
        }

    }

}