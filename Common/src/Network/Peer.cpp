#include "spdlog/spdlog.h"

#include "Network/PacketView.h"
#include "Network/Peer.h"

namespace Network {

Peer::Peer(const std::shared_ptr<Epoll::Epoll>& epl, PacketHandlerCallable packetHandler)
    : epoll(epl), packetHandler(std::move(packetHandler)) {}

Peer::~Peer() {
    if (isConnected) {
        socket->close();
    }
}

void Peer::scheduleBufferSend(std::span<const std::byte> buffer) {
    const bool shouldUpdateListener = sendBuffers.empty();

    std::vector<std::byte> buffTmp;
    buffTmp.reserve(buffer.size() + sizeof(PacketView::PACKET_SIZE_TYPE));

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
        events.data = epollExtraData;
        epoll->modifyInPool(getFd(), events);
    }
}

void Peer::setSocket(std::unique_ptr<Socket> newSocket) {
    if (socket != nullptr) {
        epoll->removeFromPool(getFd());
    }
    socket = std::move(newSocket);

    static epoll_event eventDefault = {EPOLLIN | EPOLLRDHUP | EPOLLHUP};
    eventDefault.data = epollExtraData;

    if (epoll->addIntoPool(getFd(), eventDefault) == -1) {
        throw std::system_error(errno, std::system_category(), "addIntoPool");
    }
}

bool Peer::onDataAvailable() {
    while (true) {
        size_t bytesToRead = sizeof(rBuffer) - bytesInReadingBuffer;
        int bytesReceived = this->socket->recv({rBuffer.data() + bytesInReadingBuffer, bytesToRead});

        if (bytesReceived == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            SPDLOG_ERROR("Peer::onDataAvailable: recv failed: {}", strerror(errno));
            disconnect();
            return false;
        }

        if (bytesReceived == 0) {
            SPDLOG_DEBUG("Remote peer( sockFd: {} ) sent graceful disconnect", socket->getFd());
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

                std::memcpy(&currentPacketSizeExpected, rBuffer.data() + currentReadingOffset,
                            sizeof(currentPacketSizeExpected));

                if (currentPacketSizeExpected > PacketView::MAXIMUM_PACKET_SIZE) {
                    SPDLOG_WARN("Remote peer( sockFd: {} ) announced packet with size {} "
                                "bytes what is "
                                "larger than maximum allowed {} bytes",
                                socket->getFd(), currentPacketSizeExpected, PacketView::MAXIMUM_PACKET_SIZE);

                    disconnect();
                    return false;
                }

                SPDLOG_DEBUG("Remote peer( sockFd: {} ) sent next packet announcement for next packet of {} bytes",
                             socket->getFd(), currentPacketSizeExpected);

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

            packetBuffer.insert(packetBuffer.end(), rBuffer.begin() + currentReadingOffset,
                                rBuffer.begin() + currentReadingOffset + bufferBytesGoingToRead);
            currentReadingOffset += bufferBytesGoingToRead;

            if (packetBuffer.size() == currentPacketSizeExpected) {
                auto packetView = PacketView(packetBuffer);
                if (!packetView.verify()) {
                    SPDLOG_WARN("Remote peer( sockFd: {} ) sent an invalid packet", socket->getFd());
                    disconnect();
                    return false;
                }

                auto parsedView = packetView.GetParsedView();
                auto packetType = parsedView->packet_union_type();
                SPDLOG_DEBUG("Received packet of type {} from Remote peer( sockFd: {} )", static_cast<int>(packetType),
                             socket->getFd());

                packetHandler(parsedView);

                currentPacketSizeExpected = 0; // So, next iteration we fetch new packet
            }
        }
        bytesInReadingBuffer = 0;
    }

    return true;
}

void Peer::onDataSendingAvailable() {
    while (true) {
        constexpr static int maximumBytesPerOperation = 4194304; // 4MB

        if (sendBuffers.empty()) {
            static epoll_event events = {EPOLLIN | EPOLLRDHUP | EPOLLHUP};
            events.data = epollExtraData;
            epoll->modifyInPool(getFd(), events);

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

void Peer::disconnect() {
    if (isConnected) {
        SPDLOG_DEBUG("Remote peer( sockFd: {} ) is marked as disconnected", socket->getFd());
        socket->close();
        isConnected = false;
    }
}
} // namespace Network