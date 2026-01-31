#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H
#include <cstddef>
#include <list>

#include "Network/PacketView.h"
#include "Network/Peer.h"
#include "Network/Socket.h"
#include "Services/PacketsDispatchService_/ServerPacketContext.h"
#include "Services/ServiceProvider.h"

namespace Services {
class ServerService;
}

namespace Network::Server {
class ClientConnection {
    private:
        inline static unsigned int nextConnectionId = 0;
        static unsigned int getNextConnectionId() { return nextConnectionId++; }

        Services::ServiceProvider& service_provider;

        Services::PacketsDispatchService_::ServerPacketContext dispatchCtx;

        std::unique_ptr<Peer> networkPeer;

    public:
        unsigned int connectionId;

        ClientConnection(Services::ServiceProvider& service_provider, std::unique_ptr<Socket> clientSocket);

        void scheduleBufferSend(std::span<std::byte> buffer) { return networkPeer->scheduleBufferSend(buffer); };

        bool onDataAvailable() { return networkPeer->onDataAvailable(); };

        void onDataSendingAvailable() { return networkPeer->onDataSendingAvailable(); };

        void disconnect() { return networkPeer->disconnect(); };

        /**
         * Calls peer->enable
         * * Makes peer add its initial interests into epoll
         */
        void enable() const { networkPeer->enable(); };

        int getFd() const noexcept { return networkPeer->getFd(); }

        bool isConnected = true;
};
} // namespace Network::Server
#endif
