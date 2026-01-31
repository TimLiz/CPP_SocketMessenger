#include "Services/ClientService.h"

#include "Network/PacketView.h"
#include "Services/PacketsDispatchService.h"
#include "spdlog/spdlog.h"

using namespace Services;
using namespace Network;

ClientService::ClientService(ServiceProvider& serviceProvider) : ServiceBase(serviceProvider) {
    auto& dispatchService = serviceProvider.getService<PacketsDispatchService>();
    networkPeer = std::make_unique<Peer>(epoll, [&dispatchService](const Packets::Base* packet) {
        static PacketsDispatchService_::ClientPacketContext ctx;
        dispatchService.dispatchPacket(packet, ctx);
    });
}

int ClientService::connect(const std::string_view host, const int port) {
    std::unique_ptr<Socket> socket = std::make_unique<Socket>(SocketType::SOCK_STREAM);

    auto status = socket->connect(host, port);
    if (status == -1)
        return status;

    socket->setNonBlocking();

    networkPeer->isConnected = true;

    auto basicTransport = std::make_unique<BasicTransport>(std::move(socket));
    networkPeer->setTransport(std::move(basicTransport));

    return status;
}

bool ClientService::onConnectionDied() {
    SPDLOG_CRITICAL("Lost connection to the server.");
    // TODO: Implement automatic reconnection
    return false;
}

void ClientService::run() {
    networkPeer->enable();

    isRunning = true;

    SPDLOG_DEBUG("ClientService::run");
    static constexpr int EPOLL_EVENT_BUFFER_SIZE = 128;

    std::array<epoll_event, EPOLL_EVENT_BUFFER_SIZE> epollEventsBuffer{};
    while (isRunning) {
        const int eventsCount = epoll->epoll_wait(epollEventsBuffer);
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
                if (!networkPeer->onDataAvailable()) {
                    if (!onConnectionDied()) {
                        return;
                    }
                }
            }

            if (event.events & EPOLLOUT) {
                networkPeer->onDataSendingAvailable();
            }
        }
    }
}
void ClientService::stop() { isRunning = false; }

void ClientService::scheduleDataSend(const std::span<const std::byte> buffer) const {
    networkPeer->scheduleBufferSend(buffer);
}