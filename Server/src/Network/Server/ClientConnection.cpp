#include "Network/Server/ClientConnection.h"

#include "Services/PacketsDispatchService.h"
#include "Services/ServerService.h"

#include "spdlog/spdlog.h"

namespace Network::Server {
ClientConnection::ClientConnection(Services::ServiceProvider& service_provider, std::unique_ptr<Socket> clientSocket)
    : service_provider(service_provider), dispatchCtx({*this}) {

    auto& dispatchService = service_provider.getService<Services::PacketsDispatchService>();

    auto handlerLamda = [&dispatchService, this](const Packets::Base* packet) {
        dispatchService.dispatchPacket(packet, this->dispatchCtx);
    };
    auto& serverService = service_provider.getService<Services::ServerService>();
    networkPeer = std::make_unique<Peer>(serverService.epoll, std::move(handlerLamda));

    connectionId = getNextConnectionId();

    networkPeer->setSocket(std::move(clientSocket));
}

} // namespace Network::Server
