#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H
#include <cstddef>
#include <list>

#include "../../../../Common/include/Network/Socket.h"
#include "flatbuffers/buffer.h"
#include "boost/container/static_vector.hpp"

namespace Network::Server {
    class Server;

    class ClientConnection {
        private:
            inline static unsigned int nextConnectionId = 0;

            static unsigned int getNextConnectionId() { return nextConnectionId++; }


            std::shared_ptr<Socket> socket;
            Server* relatedServer;

            std::array<std::byte, 4096> buffer{};

            /** The data is copied from the socket into buffer
             *  "currentPacketBufferPtr" might be different from "&buffer" if
             *      - Currently receiving packet size is larger than "sizeof(buffer)"
             */
            std::byte* currentPacketBufferPtr = buffer.data();
            int currentBufferOffset = 0;

            std::list<std::vector<std::byte>> sendBuffers;
        public:
            unsigned int connectionId;

            ClientConnection(std::shared_ptr<Socket> socketConnectionId, Server* server);

            void scheduleDataSend(std::span<std::byte> buffer);

            bool onDataAvailable();

            void onDataSendingAvailable();
            void disconnect();

            int getFd() const { return socket->getFd(); }

            bool isConnected = true;
    };
}
#endif
