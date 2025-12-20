#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H
#include <cstddef>
#include <list>

#include "flatbuffers/buffer.h"
#include "boost/container/static_vector.hpp"

namespace Network::Server {
    class Server;

    class ClientConnection {
        private:
            int socketConnectionId;
            Server* relatedServer;

            std::byte buffer[4096];

            /** The data is copied from the socket into buffer
             *  "currentPacketBufferPtr" might be different from "&buffer" if
             *      - Currently receiving packet size is larger than "sizeof(buffer)"
             */
            std::byte* currentPacketBufferPtr = buffer;
            int currentBufferOffset = 0;

            std::list<std::vector<std::byte>> sendBuffers;
        public:
            ClientConnection(int socketConnectionId, Server* server);
            ~ClientConnection();

            void scheduleDataSend(std::span<std::byte> buffer);
            void onDataAvailable();
            void onDataSendingAvailable();
            void disconnect();

            bool isDisconnected = false;
    };
}
#endif
