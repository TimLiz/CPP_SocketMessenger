#include "Network/Server/ClientConnection.h"

#ifdef ENABLE_SECURE_TRANSPORT
#include "Network/Transport/SecureTransport.h"
#else
#include "Network/Transport/BasicTransport.h"
#endif

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

    networkPeer->setEpollData({.u32 = connectionId});

#ifdef ENABLE_SECURE_TRANSPORT
    auto transport = std::make_unique<SecureTransport>(std::move(clientSocket));
#else
    auto transport = std::make_unique<BasicTransport>(std::move(clientSocket));
#endif
    networkPeer->setTransport(std::move(transport));

    SPDLOG_DEBUG("Created new ClientConnection ( fd: {}, connId: {} )", networkPeer->getFd(), connectionId);
}

} // namespace Network::Server
