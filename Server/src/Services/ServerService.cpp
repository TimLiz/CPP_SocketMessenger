#include "Services/ServerService.h"
#include "Services/ServiceProvider.h"

using namespace Network;

namespace Services {
void ServerService::clientsProcessingThreadEntryPoint() {
    SPDLOG_DEBUG("Server::clientsProcessingThreadEntryPoint");
    static constexpr int EPOLL_EVENT_BUFFER_SIZE = 128;

    std::array<epoll_event, EPOLL_EVENT_BUFFER_SIZE> epollEventsBuffer{};
    while (true) {
        const int eventsCount = epoll->epoll_wait(epollEventsBuffer);
        if (eventsCount == -1) {
            throw std::system_error(errno, std::system_category(), "epoll_wait");
        }

        for (int i = 0; i < eventsCount; i++) {
            auto event = epollEventsBuffer[i];

            std::shared_ptr<Server::ClientConnection> clientConnection;
            {
                auto it = clients.find(event.data.u32);
                if (it == clients.end()) {
                    SPDLOG_ERROR("Received epoll event for client {}, but this client is not found.", event.events);
                    continue;
                }

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

void ServerService::processClient(std::unique_ptr<Socket> clientSocket) {
    SPDLOG_DEBUG("Processing new client ( sockFD {} )", clientSocket->getFd());
    if (clientSocket->setNonBlocking() == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to set client non-blocking");
    }

    auto newClient = new Server::ClientConnection(_services, std::move(clientSocket));
    clients.emplace(newClient->connectionId, newClient);
}

void ServerService::processClientDisconnect(std::shared_ptr<Server::ClientConnection> connection) {
    epoll->removeFromPool(connection->getFd());
    connection->disconnect();

    clients.erase(connection->connectionId);
    SPDLOG_DEBUG("Client connection {} completely disconnected", connection->connectionId);
}

ServerService::ServerService(ServiceProvider& serviceProvider)
    : ServiceBase(serviceProvider), socket(SocketType::SOCK_STREAM) {
    if (socket.bindLoopback(25365) == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to bind socket");
    }

    SPDLOG_DEBUG("Server bound successfully");
}

void ServerService::run() {
    if (socket.listen() == -1) {
        throw std::system_error(errno, std::system_category(), "Failed to listen on socket");
    }

    auto thread = std::thread(&ServerService::clientsProcessingThreadEntryPoint, this);

    SPDLOG_INFO("Server started");
    while (true) {
        auto acceptedConnection = socket.accept();
        if (acceptedConnection == nullptr) {
            throw std::system_error(errno, std::system_category(), "Failed to accept new connection");
        }

        try {
            this->processClient(std::move(acceptedConnection));
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Client processing failed: {}", e.what());
            return;
        }
    }
}
} // namespace Services
