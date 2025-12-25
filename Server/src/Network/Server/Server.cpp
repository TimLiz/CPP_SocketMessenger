#include "../../../include/Network/Server/Server.h"

#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>
#include <unistd.h>

#include "sys/socket.h"
#include "netinet/in.h"
#include "sys/epoll.h"
#include "sys/fcntl.h"

#include "Network/Server/ClientConnection.h"

namespace Network::Server {
    void Server::clientsProcessingThreadEntryPoint() {
        static constexpr int EPOLL_EVENT_BUFFER_SIZE = 128;


        std::array<epoll_event, EPOLL_EVENT_BUFFER_SIZE> epollEventsBuffer{};
        while (true) {
            const int eventsCount = epoll.epoll_wait(epollEventsBuffer);
            if (eventsCount == -1) {
                throw std::system_error(errno, std::system_category(), "epoll_wait");
            }

            for (int i = 0; i < eventsCount; i++) {
                auto event = epollEventsBuffer[i];

                std::shared_ptr<ClientConnection> clientConnection; {
                    auto it = clients.find(event.data.u32);
                    if (it == clients.end()) continue;

                    clientConnection = it->second;
                }

                if (event.events & EPOLLRDHUP or event.events & EPOLLHUP) {
                    processClientDisconnect(clientConnection);
                    break;
                }

                if (event.events & EPOLLIN) {
                    if (!clientConnection->onDataAvailable()) {
                        processClientDisconnect(clientConnection);
                        break;
                    }
                }

                if (event.events & EPOLLOUT) {
                    clientConnection->onDataSendingAvailable();
                }
            }
        }
    }

    void Server::processClient(std::shared_ptr<Socket> clientSocket) {
        if (clientSocket->setNonBlocking() == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to set client non-blocking");
        }

        auto newClient = new ClientConnection(clientSocket, this);
        clients.emplace(newClient->connectionId, newClient);

        static epoll_event epollEvent_forNewConns = {
            EPOLLIN | EPOLLRDHUP | EPOLLHUP,
            nullptr
        };

        epollEvent_forNewConns.data.u32 = newClient->connectionId;
        if (epoll.addIntoPool(clientSocket->getFd(), epollEvent_forNewConns) == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to add in pool");
        }
    }

    void Server::processClientDisconnect(std::shared_ptr<ClientConnection> connection) {
        epoll.removeFromPool(connection->getFd());
        connection->disconnect();

        clients.erase(connection->connectionId);
    }

    Server::Server(): socket(SocketType::SOCK_STREAM) {
        if (socket.bindLoopback(25365) == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to bind socket");
        }
    }

    Server::~Server() {
        // TODO: Clean up server socket
    }

    void Server::run() {
        if (socket.listen() == -1) {
            throw std::system_error(errno, std::system_category(), "Failed to listen on socket");
        }

        auto thread = std::thread(&Server::clientsProcessingThreadEntryPoint, this);
        while (true) {
            auto acceptedConnection = socket.accept();
            if (acceptedConnection == nullptr) {
                throw std::system_error(errno, std::system_category(), "Failed to accept new connection");
            }

            this->processClient(acceptedConnection);
        }
    }
}
