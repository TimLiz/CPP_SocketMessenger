#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H
#include <cstddef>
#include <list>

#include "../../../../Common/include/Network/PacketView.h"
#include "../../../../Common/include/Network/Socket.h"
#include "flatbuffers/buffer.h"
#include "boost/container/static_vector.hpp"

namespace Services {
    class ServerService;
}

namespace Network::Server {
    class ClientConnection {
        private:
            inline static unsigned int nextConnectionId = 0;

            static unsigned int getNextConnectionId() { return nextConnectionId++; }


            std::shared_ptr<Socket> socket;
            std::shared_ptr<Services::ServerService> relatedServer;
            std::array<std::byte, 16000> buffer{};
            int bytesInReadingBuffer = 0; // Amount of bytes in reading buffer
            PacketView::PACKET_SIZE_TYPE currentPacketSizeExpected = 0;
            std::vector<std::byte> packetBuffer; // Stores all chunks of current packet
            std::list<std::vector<std::byte>> sendBuffers;


        public:
            unsigned int connectionId;

            ClientConnection(std::shared_ptr<Socket> socketConnectionId,
                             const std::shared_ptr<Services::ServerService>& server);

            void scheduleDataSend(std::span<std::byte> buffer);

            bool onDataAvailable();

            void onDataSendingAvailable();
            void disconnect();

            int getFd() const { return socket->getFd(); }

            bool isConnected = true;
    };
}
#endif
