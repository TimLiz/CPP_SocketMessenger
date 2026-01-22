#include "Services/ClientService.h"

#include "Network/PacketView.h"
#include "Services/PacketsDispatchService.h"
#include "spdlog/spdlog.h"

using namespace Services;
using namespace Network;

ClientService::ClientService(ServiceProvider& serviceProvider)
    : ServiceBase(serviceProvider), socket(Network::SocketType::SOCK_STREAM) {

    static epoll_event eventDefault = {EPOLLIN | EPOLLRDHUP | EPOLLHUP};
    if (epoll.addIntoPool(socket.getFd(), eventDefault) == -1) {
        throw std::system_error(errno, std::system_category(), "addIntoPool");
    }
}
int ClientService::connect(const std::string_view host, const int port) {
    const auto ret = socket.connect(host, port);
    if (ret != -1)
        isConnected = true;

    return ret;
}

bool ClientService::onConnectionDied() {
    SPDLOG_CRITICAL("Lost connection to the server.");
    // TODO: Implement automatic reconnection
    return false;
}

void ClientService::scheduleDataSend(std::span<std::byte> buffer) {
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

        epoll.modifyInPool(this->socket.getFd(), events);
    }
}

bool ClientService::onDataAvailable() {
    while (true) {
        size_t bytesToRead = sizeof(buffer) - bytesInReadingBuffer;
        int bytesReceived = this->socket.recv({buffer.data() + bytesInReadingBuffer, bytesToRead});

        if (bytesReceived == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            SPDLOG_ERROR("ClientService::onDataAvailable: recv failed: {}", strerror(errno));
            disconnect();
            return false;
        }
        if (bytesReceived == 0) {
            SPDLOG_DEBUG("Received graceful disconnect request from server");
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
                    SPDLOG_WARN("Server sent packet with size {} bytes what is larger than maximum allowed {} bytes",
                                currentPacketSizeExpected, PacketView::MAXIMUM_PACKET_SIZE);
                    disconnect();
                    return false;
                }

                SPDLOG_DEBUG("Received next packet size from server: {} bytes", currentPacketSizeExpected);
                packetBuffer.reserve(currentPacketSizeExpected);
                packetBuffer.resize(0);

                currentReadingOffset += sizeof(currentPacketSizeExpected);
                continue;
            }

            // After size is obtained and buffer is reserved we can fetch packet body
            unsigned int packetBytesLeftToRead = currentPacketSizeExpected - packetBuffer.size();
            unsigned int bytesLeftInBuffer = bytesInReadingBuffer - currentReadingOffset;
            unsigned int bufferBytesGoingToRead = std::min(packetBytesLeftToRead, bytesLeftInBuffer);

            packetBuffer.insert(packetBuffer.end(), buffer.begin() + currentReadingOffset,
                                buffer.begin() + currentReadingOffset + bufferBytesGoingToRead);
            currentReadingOffset += bufferBytesGoingToRead;

            if (packetBuffer.size() == currentPacketSizeExpected) {
                auto packetView = PacketView(packetBuffer);
                if (!packetView.verify()) {
                    SPDLOG_WARN("Inbound packet from server failed.");
                    disconnect();
                    return false;
                }

                auto parsedView = packetView.GetParsedView();
                auto packetType = parsedView->packet_union_type();
                SPDLOG_WARN("Received packet with type {} from client", (int)packetType);

                static PacketsDispatchService_::ClientPacketContext ctx = {};
                _services.getService<PacketsDispatchService>().dispatchPacket(parsedView, ctx);

                currentPacketSizeExpected = 0; // So, next iteration we fetch new packet
            }
        }
    }

    return true;
}
void ClientService::onDataSendingAvailable() {
    while (true) {
        constexpr static int maximumBytesPerOperation = 4194304; // 4MB

        if (sendBuffers.empty()) {
            static epoll_event events = {EPOLLIN | EPOLLRDHUP | EPOLLHUP};

            epoll.modifyInPool(this->socket.getFd(), events);
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

        int bytesSent = this->socket.send(iovecs);
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

void ClientService::run() {
    SPDLOG_DEBUG("ClientService::run");
    static constexpr int EPOLL_EVENT_BUFFER_SIZE = 128;

    std::array<epoll_event, EPOLL_EVENT_BUFFER_SIZE> epollEventsBuffer{};
    while (true) {
        const int eventsCount = epoll.epoll_wait(epollEventsBuffer);
        if (eventsCount == -1) {
            throw std::system_error(errno, std::system_category(), "epoll_wait");
        }

        for (int i = 0; i < eventsCount; i++) {
            auto event = epollEventsBuffer[i];

            if (event.events & EPOLLRDHUP or event.events & EPOLLHUP) {
                if (!onConnectionDied()) {
                    return;
                }
            }

            if (event.events & EPOLLIN) {
                if (!onDataAvailable()) {
                    if (!onConnectionDied()) {
                        return;
                    }
                }
            }

            if (event.events & EPOLLOUT) {
                onDataSendingAvailable();
            }
        }
    }
}