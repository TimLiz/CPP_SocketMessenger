#include "spdlog/spdlog.h"

#include "Network/PacketView.h"
#include "Network/Peer.h"

namespace Network {

Peer::Peer(const std::shared_ptr<Epoll::Epoll>& epl, PacketHandlerCallable packetHandler)
    : epoll(epl), packetHandler(std::move(packetHandler)) {}

Peer::~Peer() { disconnect(); }

void Peer::trySyncEpollInterests() const {
    if (!isEnabled) // In case if called before Peer.enable()
        return;

    if (epoll_event epollEvents; transport->tryGetEpollInterests(epollEvents.events)) {
        epollEvents.data = epollExtraData;

        if (epoll->modifyInPool(getFd(), epollEvents) == -1) {
            SPDLOG_WARN("Failed to modify epoll interests for fd {}, err: {}", getFd(), strerror(errno));
        }
    }
}

void Peer::schedulePacketSend(const flatbuffers::FlatBufferBuilder& builder) const {
    const std::byte* builderBuffPtr = reinterpret_cast<std::byte*>(builder.GetBufferPointer());
    const auto builderBuffSize = builder.GetSize();

    std::vector<std::byte> buffTmp;
    buffTmp.reserve(builderBuffSize + sizeof(PacketView::PACKET_SIZE_TYPE));

    // Packet size
    {
        PacketView::PACKET_SIZE_TYPE bufferSize = builderBuffSize;
        auto p = reinterpret_cast<std::byte*>(&bufferSize);

        buffTmp.insert(buffTmp.begin(), p, p + sizeof(bufferSize));
    }

    buffTmp.insert(buffTmp.begin() + sizeof(PacketView::PACKET_SIZE_TYPE), builderBuffPtr,
                   builderBuffPtr + builderBuffSize);

    transport->scheduleBufferSend(std::move(buffTmp));

    trySyncEpollInterests();
}

void Peer::setTransport(std::unique_ptr<ITransport> newTransport) {
    if (isEnabled) {
        epoll->removeFromPool(getFd());
        isEnabled = false;
    }

    transport = std::move(newTransport);
}

void Peer::enable() {
    isEnabled = true;

    epoll_event epollEvents;
    epollEvents.data = epollExtraData;
    if (transport->tryGetEpollInterests(epollEvents.events)) {
        epollEvents.data = epollExtraData;
    } else {
        epollEvents.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
    }

    if (epoll->addIntoPool(getFd(), epollEvents) == -1) {
        throw std::system_error(errno, std::system_category(), "addIntoPool");
    }
}

bool Peer::onDataAvailable() {
    const bool isTransportReadAvailable = transport->onDataReadAvailable();

    while (isTransportReadAvailable) {
        size_t bytesToRead = sizeof(rBuffer) - bytesInReadingBuffer;
        int bytesReceived = transport->read({rBuffer.data() + bytesInReadingBuffer, bytesToRead});

        if (bytesReceived == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            SPDLOG_ERROR("Peer::onDataAvailable: recv failed: {}", strerror(errno));
            disconnect();
            return false;
        }

        if (bytesReceived == 0) {
            SPDLOG_DEBUG("Remote peer( sockFd: {} ) sent graceful disconnect", getFd());
            disconnect();
            return false;
        }
        bytesInReadingBuffer += bytesReceived;

        unsigned int currentReadingOffset = 0;
        while (currentReadingOffset != bytesInReadingBuffer) {
            if (currentPacketSizeExpected == 0) {
                // Then fetch current packet size
                if (bytesInReadingBuffer < sizeof(currentPacketSizeExpected))
                    return true;

                std::memcpy(&currentPacketSizeExpected, rBuffer.data() + currentReadingOffset,
                            sizeof(currentPacketSizeExpected));

                if (currentPacketSizeExpected > PacketView::MAXIMUM_PACKET_SIZE) {
                    SPDLOG_WARN("Remote peer( sockFd: {} ) announced packet with size {} "
                                "bytes what is "
                                "larger than maximum allowed {} bytes",
                                getFd(), currentPacketSizeExpected, PacketView::MAXIMUM_PACKET_SIZE);

                    disconnect();
                    return false;
                }

                SPDLOG_DEBUG("Remote peer( sockFd: {} ) sent next packet announcement for next packet of {} bytes",
                             getFd(), currentPacketSizeExpected);

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
                    SPDLOG_WARN("Remote peer( sockFd: {} ) sent an invalid packet", getFd());
                    disconnect();
                    return false;
                }

                auto parsedView = packetView.GetParsedView();
                auto packetType = parsedView->packet_union_type();
                if (packetType == Packets::Packets_NONE) {
                    SPDLOG_WARN("Received packet of type NONE from Remote peer( sockFd: {} ). Closing connection.",
                                getFd());
                    disconnect();
                    return false;
                }

                SPDLOG_DEBUG("Received packet of type {} from Remote peer( sockFd: {} )", static_cast<int>(packetType),
                             getFd());

                packetHandler(parsedView);

                currentPacketSizeExpected = 0; // So, next iteration we fetch new packet
            }
        }
        bytesInReadingBuffer = 0;
    }

    trySyncEpollInterests();
    return true;
}

void Peer::onDataSendingAvailable() const {
    transport->onDataSendingAvailable();

    trySyncEpollInterests();
}

// We don't need to remove interests from epoll here, right?
void Peer::disconnect() {
    if (isConnected) {
        SPDLOG_DEBUG("Remote peer( sockFd: {} ) is marked as disconnected", getFd());

        if (transport->disconnect() == -1) {
            SPDLOG_WARN("Failed to release socket FD {}", getFd());
        }
        isConnected = false;
    }
}
} // namespace Network