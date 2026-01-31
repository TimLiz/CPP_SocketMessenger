#include "Network/Transport/BasicTransport.h"

#include "Network/PacketView.h"

#include "spdlog/spdlog.h"

#include "sys/epoll.h"

namespace Network {

BasicTransport::BasicTransport(std::unique_ptr<Socket> socket) : socket(std::move(socket)) {}

bool BasicTransport::tryGetEpollInterests(__poll_t& dest) noexcept {
    if (!didEpollInterestsChanged)
        return false;
    didEpollInterestsChanged = false;
    dest = currentEpollInterests;

    return true;
}

void BasicTransport::scheduleBufferSend(std::vector<std::byte> buffer) {
    const bool shouldUpdateListener = sendBuffers.empty();

    sendBuffers.push_back(std::move(buffer));

    if (shouldUpdateListener) {
        modifyEventInterests(EPOLLOUT, true);
    }
}

void BasicTransport::onDataSendingAvailable() {
    while (true) {
        constexpr static int maximumBytesPerOperation = 4194304; // 4MB

        if (sendBuffers.empty()) {
            modifyEventInterests(EPOLLOUT, false);
            return;
        }

        // Building iovecs
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

            SPDLOG_ERROR("BasicTransport::onDataSendingAvailable: send failed: {}", strerror(errno));
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

int BasicTransport::read(const std::span<std::byte> readBuffer) { return socket->recv(readBuffer); }

int BasicTransport::disconnect() {
    if (isConnected) {
        SPDLOG_DEBUG("Basic transport( sockFd: {} ) is marked as disconnected", getFd());

        const auto ret = socket->close();

        if (ret == -1) {
            SPDLOG_WARN("Failed to release socket FD {}", getFd());
        }
        isConnected = false;

        return ret;
    }

    return -1;
}

void BasicTransport::modifyEventInterests(const __poll_t interests, const bool isInterested) {
    if (isInterested) {
        currentEpollInterests |= interests;
    } else {
        currentEpollInterests &= ~interests;
    }

    didEpollInterestsChanged = true;
}

} // namespace Network