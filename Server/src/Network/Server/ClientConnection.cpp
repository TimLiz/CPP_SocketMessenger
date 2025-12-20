#include "../../../include/Network/Server/ClientConnection.h"

#include <iostream>
#include <ostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "Network/Server/Server.h"

namespace Network::Server {
    ClientConnection::ClientConnection(int socketConnectionId, Server* server):socketConnectionId(socketConnectionId), relatedServer(server) {}

    ClientConnection::~ClientConnection() {
        close(socketConnectionId); // Must be closed in destructor as I use it as client identifier
    }

    void ClientConnection::scheduleDataSend(std::span<std::byte> buffer) {
        const bool shouldUpdateListener = sendBuffers.empty();

        sendBuffers.emplace_back(buffer.begin(), buffer.end());

        if (shouldUpdateListener) {
            static epoll_event events = {EPOLLIN | EPOLLRDHUP | EPOLLOUT};
            events.data.ptr = this;

            relatedServer->setEpollEventForConnection(this->socketConnectionId, events);
        }
    }

    void ClientConnection::onDataAvailable() {
        if (isDisconnected) return;

        while (true) {
            int bufferCapacity = sizeof(buffer) - currentBufferOffset;
            int bytesReceived = recv(socketConnectionId, buffer + currentBufferOffset, bufferCapacity, MSG_DONTWAIT);

            if (bytesReceived == 0) {
                disconnect();
                return;
            }

            if (bytesReceived == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }

                std::cerr << "recv() failed: " << strerror(errno) << std::endl;
                disconnect();
                return;
            }

            // currentBufferOffset += bytesReceived;
            //
            // if (currentBufferOffset == 4) {
            //
            // }
        }

        {
            std::string str = "HTTP/1.1 200 OK\nContent-Length: 12\r\n\r\nHello there!";

            std::span byteSpan(reinterpret_cast<std::byte*>(str.data()), str.size());
            this->scheduleDataSend(byteSpan);
        }
    }

    void ClientConnection::onDataSendingAvailable() {
        if (isDisconnected) return;

        while (true) {
            constexpr static int maximumBytesPerOperation = 4194304; // 4MB

            if (sendBuffers.empty()) {
                static epoll_event events = {EPOLLIN | EPOLLRDHUP};
                events.data.ptr = this;

                relatedServer->setEpollEventForConnection(this->socketConnectionId, events);
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

            int bytesSent = writev(socketConnectionId, iovecs.data(), iovecs.size());
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
        if (isDisconnected) return;

        static epoll_event emptyEvent = {};
        this->relatedServer->setEpollEventForConnection(this->socketConnectionId, emptyEvent);

        isDisconnected = true;
        this->relatedServer->clientConnectionsToDisconnect.insert(socketConnectionId);
    }
}
