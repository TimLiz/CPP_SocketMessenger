#include "../../../include/Network/Server/ClientConnection.h"

#include <iostream>
#include <ostream>
#include <utility>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "Network/Server/Server.h"

#include "boost/format.hpp"

namespace Network::Server {
    ClientConnection::ClientConnection(std::shared_ptr<Socket> socketConnectionId, Server* server) {
        socket = std::move(socketConnectionId);
        relatedServer = server;
        connectionId = getNextConnectionId();
    }

    void ClientConnection::scheduleDataSend(std::span<std::byte> buffer) {
        const bool shouldUpdateListener = sendBuffers.empty();

        sendBuffers.emplace_back(buffer.begin(), buffer.end());

        if (shouldUpdateListener) {
            static epoll_event events = {EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLOUT};
            events.data.u32 = this->connectionId;

            relatedServer->epoll.modifyInPool(this->socket->getFd(), events);
        }
    }

    /**
     *
     * @return False if connection was closed
     */
    bool ClientConnection::onDataAvailable() {
        while (true) {
            size_t bufferCapacity = sizeof(buffer) - currentBufferOffset;
            int bytesReceived = this->socket->recv({buffer.data() + currentBufferOffset, bufferCapacity});

            if (bytesReceived == 0) {
                disconnect();
                return false;
            }

            if (bytesReceived == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }

                std::cerr << "recv() failed: " << strerror(errno) << std::endl;
                disconnect();
                return false;
            }

            currentBufferOffset += bytesReceived;
        }

        return true;
    }

    void ClientConnection::onDataSendingAvailable() {
        while (true) {
            constexpr static int maximumBytesPerOperation = 4194304; // 4MB

            if (sendBuffers.empty()) {
                static epoll_event events = {EPOLLIN | EPOLLRDHUP | EPOLLHUP};
                events.data.u32 = this->connectionId;

                relatedServer->epoll.modifyInPool(this->socket->getFd(), events);
                return;
            }

            std::vector<iovec> iovecs;
            int operationBytesLeft = maximumBytesPerOperation;
            for (auto& currentBuffer : sendBuffers) {
                if (operationBytesLeft == 0) {
                    break;
                }

                int bufferBytesToSend = std::min(static_cast<size_t>(operationBytesLeft), currentBuffer.size());
                iovecs.emplace_back(iovec{currentBuffer.data(), static_cast<size_t>(bufferBytesToSend)});
                operationBytesLeft -= bufferBytesToSend;
            }

            int bytesSent = this->socket->send(iovecs);
            if (bytesSent == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return;
                }

                std::cerr << "write() failed: " << strerror(errno) << std::endl;
                disconnect();
                return;
            }

            auto currentBufferIt = sendBuffers.begin();
            while (currentBufferIt != sendBuffers.end()) {
                if (currentBufferIt->size() <= bytesSent) {
                    bytesSent -= currentBufferIt->size();
                    currentBufferIt = sendBuffers.erase(currentBufferIt);
                    continue;
                }

                currentBufferIt->erase(currentBufferIt->begin(), currentBufferIt->begin() + bytesSent);
                break;
            }

            if (bytesSent == 0) {
                return;
            }
        }
    }

    void ClientConnection::disconnect() {
        isConnected = false;
    }
}
