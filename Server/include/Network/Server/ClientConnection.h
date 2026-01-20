#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H
#include <cstddef>
#include <list>

#include "Network/PacketView.h"
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
        std::shared_ptr<Socket> socket;
        std::array<std::byte, 16000> buffer{};
        int bytesInReadingBuffer = 0; // Amount of bytes in reading buffer
        PacketView::PACKET_SIZE_TYPE currentPacketSizeExpected = 0;
        std::vector<std::byte> packetBuffer; // Stores all chunks of current packet
        std::list<std::vector<std::byte>> sendBuffers;

        Services::PacketsDispatchService_::ServerPacketContext dispatchCtx;

    public:
        unsigned int connectionId;

        ClientConnection(Services::ServiceProvider& service_provider, std::shared_ptr<Socket> socketConnectionId);

        void scheduleDataSend(std::span<std::byte> buffer);

        bool onDataAvailable();

        void onDataSendingAvailable();
        void disconnect();

        void dispatchPacket(const Packets::Base* packet);

        int getFd() const { return socket->getFd(); }

        bool isConnected = true;
};
} // namespace Network::Server
#endif
