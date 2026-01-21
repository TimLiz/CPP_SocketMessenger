#include "Network/Server/ClientConnection.h"

#include <sys/epoll.h>
#include <sys/socket.h>

#include <iostream>
#include <utility>

#include "Network/PacketView.h"
#include "Services/PacketsDispatchService.h"
#include "Services/ServerService.h"
#include "spdlog/spdlog.h"

namespace Network::Server {
ClientConnection::ClientConnection(Services::ServiceProvider& service_provider,
                                   std::shared_ptr<Socket> socketConnectionId)
    : service_provider(service_provider), dispatchCtx({*this}) {

    socket = std::move(socketConnectionId);
    connectionId = getNextConnectionId();
}

    void ClientConnection::scheduleDataSend(std::span<std::byte> buffer) {
        const bool shouldUpdateListener = sendBuffers.empty();

    std::vector<std::byte> buffTmp;
    buffTmp.resize(buffer.size() + sizeof(PacketView::PACKET_SIZE_TYPE));

    // Packet size
    {
        PacketView::PACKET_SIZE_TYPE bufferSize = buffer.size();
        auto p = reinterpret_cast<std::byte*>(&bufferSize);

        buffTmp.insert(buffTmp.begin(), p, p + sizeof(bufferSize));
    }

    buffTmp.insert(buffTmp.begin() + sizeof(PacketView::PACKET_SIZE_TYPE), buffer.begin(), buffer.end());

    sendBuffers.push_back(std::move(buffTmp));

        if (shouldUpdateListener) {
            static epoll_event events = {EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLOUT};
            events.data.u32 = this->connectionId;

            service_provider.getService<Services::ServerService>().epoll.modifyInPool(this->socket->getFd(), events);
        }
    }

    /**
     *
     * @return False if connection was closed
     */
    bool ClientConnection::onDataAvailable() {
        while (true) {
            size_t bytesToRead = sizeof(buffer) - bytesInReadingBuffer;
            int bytesReceived = this->socket->recv({buffer.data() + bytesInReadingBuffer, bytesToRead});

            if (bytesReceived == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }

                SPDLOG_ERROR("ClientConnection::onDataAvailable: recv failed: {}", strerror(errno));
                disconnect();
                return false;
            }
            if (bytesReceived == 0) {
                SPDLOG_DEBUG("Received gracefull disconnect request for connection {}", connectionId);
                disconnect();
                return false;
            }
            bytesInReadingBuffer += bytesReceived;

            unsigned int currentReadingOffset = 0;
            while (currentReadingOffset != bytesInReadingBuffer) {
                if (currentPacketSizeExpected == 0) {
                    // Then fetch current packet size
                    if (bytesInReadingBuffer < sizeof(currentPacketSizeExpected))
                        break;
                    std::memcpy(&currentPacketSizeExpected, buffer.data() + currentReadingOffset,
                                sizeof(currentPacketSizeExpected));

                    if (currentPacketSizeExpected > PacketView::MAXIMUM_PACKET_SIZE) {
                        SPDLOG_WARN("Client connection {} sent packet with size {} "
                                    "bytes what is "
                                    "larger than maximum allowed {} bytes",
                                    connectionId, currentPacketSizeExpected, PacketView::MAXIMUM_PACKET_SIZE);
                        disconnect();
                        return false;
                    }

                    SPDLOG_DEBUG("Received next packet size for client connection "
                                 "{} of {} bytes",
                                 connectionId, currentPacketSizeExpected);
                    packetBuffer.reserve(currentPacketSizeExpected);
                    packetBuffer.resize(0);

                    currentReadingOffset += sizeof(currentPacketSizeExpected);
                    continue;
                }

                // After size is obtained and buffer is reserved we can fetch packet
                // body
                unsigned int packetBytesLeftToRead = currentPacketSizeExpected - packetBuffer.size();
                unsigned int bytesLeftInBuffer = bytesInReadingBuffer - currentReadingOffset;
                unsigned int bufferBytesGoingToRead = std::min(packetBytesLeftToRead, bytesLeftInBuffer);

                packetBuffer.insert(packetBuffer.end(), buffer.begin() + currentReadingOffset,
                                    buffer.begin() + currentReadingOffset + bufferBytesGoingToRead);
                currentReadingOffset += bufferBytesGoingToRead;

                if (packetBuffer.size() == currentPacketSizeExpected) {
                    auto packetView = PacketView(packetBuffer);
                    if (!packetView.verify()) {
                        SPDLOG_WARN("Inbound verification for client connection {} failed.", connectionId);
                        disconnect();
                        return false;
                    }

                    auto parsedView = packetView.GetParsedView();
                    auto packetType = parsedView->packet_union_type();
                    SPDLOG_WARN("Received packet with type {} from client", (int)packetType);

                dispatchPacket(parsedView);

                    currentPacketSizeExpected = 0; // So, next iteration we fetch new packet
                }
            }
        }

        return true;
    }

    void ClientConnection::onDataSendingAvailable() {
        while (true) {
            constexpr static int maximumBytesPerOperation = 4194304; // 4MB

            if (sendBuffers.empty()) {
                static epoll_event events = {EPOLLIN | EPOLLRDHUP | EPOLLHUP};
                events.data.u32 = this->connectionId;

                service_provider.getService<Services::ServerService>().epoll.modifyInPool(this->socket->getFd(),
                                                                                          events);
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

                SPDLOG_ERROR("ClientConnection::onDataSendingAvailable: send failed: {}", strerror(errno));
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
    if (isConnected) {
        SPDLOG_DEBUG("Client connection {} is marked as disconnected", connectionId);
        isConnected = false;
    }
}

void ClientConnection::dispatchPacket(const Packets::Base* packet) {
    service_provider.getService<Services::PacketsDispatchService>().dispatchPacket(packet, dispatchCtx);
}
} // namespace Network::Server
