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
        epoll_event epollEventsBuffer[EPOLL_EVENT_BUFFER_SIZE];
        while (true) {
            const int eventsCount = epoll_wait(clientConnectionsEpoll, epollEventsBuffer, EPOLL_EVENT_BUFFER_SIZE, -1);
            if (eventsCount == -1) {
                if (errno == EINTR) {
                    continue;
                }
                throw std::system_error(errno, std::system_category(), "epoll_wait");
            }

            for (int i = 0; i < eventsCount; i++) {
                auto event = epollEventsBuffer[i];
                auto clientConnection = static_cast<ClientConnection*>(event.data.ptr);

                if (event.events & EPOLLRDHUP) {
                    clientConnection->disconnect();
                    break;
                }

                if (event.events & EPOLLIN) {
                    clientConnection->onDataAvailable();
                }

                if (event.events & EPOLLOUT) {
                    clientConnection->onDataSendingAvailable();
                }
            }

            for (auto toDeleteIdIt = this->clientConnectionsToDisconnect.begin(); toDeleteIdIt != this->clientConnectionsToDisconnect.end();) {
                this->serverConnections.erase(*toDeleteIdIt);
                toDeleteIdIt = this->clientConnectionsToDisconnect.erase(toDeleteIdIt);
            }
        }
    }

    Server::Server() {
        clientConnectionsEpoll = epoll_create1(0);

        socketId = socket(AF_INET, SOCK_STREAM, 0);
        if (socketId < 0) {
            throw std::runtime_error("Could not create socket");
        }

        sockaddr_in serverAddr {};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(25365);

        if (bind(socketId, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
            throw std::runtime_error("Could not bind socket");
        }

        if (listen(socketId, SOMAXCONN) < 0) {
            throw std::runtime_error("Could not listen on socket");
        }

        auto thread = std::thread(&Server::clientsProcessingThreadEntryPoint, this);

        while (true) {
            const int clientConnection = accept(socketId, nullptr, nullptr);
            if (clientConnection < 0) {
                break;
            }

            auto newClientRaw = serverConnections.emplace(clientConnection, std::make_unique<ClientConnection>(clientConnection, this)).first->second.get();

            static epoll_event events = {EPOLLIN | EPOLLRDHUP };
            events.data.ptr = newClientRaw;

            if (fcntl(clientConnection, F_SETFL, O_NONBLOCK) == -1) {
                throw std::system_error(errno, std::system_category(), "Failed to make client connection nonblocking");
            }

            setEpollEventForConnection(clientConnection, events);
        }
        std::cout << "Accept failed!" << std::endl;
    }

    Server::~Server() {
        if (socketId != -1) {
            epoll_ctl(clientConnectionsEpoll, EPOLL_CTL_DEL, socketId, nullptr);
            close(socketId);
        }
    }

    void Server::setEpollEventForConnection(int connectionId, epoll_event& event) const {
        if (event.events == 0) {
            if (epoll_ctl(clientConnectionsEpoll, EPOLL_CTL_DEL, connectionId, nullptr) == -1) {
                throw std::system_error(errno, std::system_category(), "epoll_ctl EPOLL_CTL_DEL failed");
            }
            return;
        }

        const bool isAddFailed = epoll_ctl(clientConnectionsEpoll, EPOLL_CTL_ADD, connectionId, &event) == -1;

        if (isAddFailed) {
            if (errno == EEXIST) {
                if (epoll_ctl(clientConnectionsEpoll, EPOLL_CTL_MOD, connectionId, &event) == -1) {
                    throw std::system_error(errno, std::system_category(), "epoll_ctl EPOLL_CTL_MOD failed");
                }
                return;
            }

            throw std::system_error(errno, std::system_category(), "epoll_ctl EPOLL_CTL_ADD failed");
        }
    }
}
