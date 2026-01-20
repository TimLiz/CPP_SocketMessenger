#ifndef MYMESSENGER_CLIENTSERVICE_H
#define MYMESSENGER_CLIENTSERVICE_H
#include "Epoll.h"
#include "Network/Socket.h"
#include "Services/ServiceBase.h"

#include "Network/PacketView.h"
#include "list"

// TODO: May be make something like Peer class to put functions like scheduleSend and packet fetching ones into to
// remove all this duplicated code
namespace Services {
class ClientService : public ServiceBase {
    private:
        Network::Socket socket;
        Epoll::Epoll epoll;

        std::list<std::vector<std::byte>> sendBuffers;

        std::array<std::byte, 16000> buffer{};
        int bytesInReadingBuffer = 0; // Amount of bytes in reading buffer
        Network::PacketView::PACKET_SIZE_TYPE currentPacketSizeExpected = 0;
        std::vector<std::byte> packetBuffer; // Stores all chunks of current packet

        bool isConnected = false;

    public:
        ClientService(ServiceProvider& serviceProvider);

        int connect(std::string_view host, int port);
        int disconnect() { return socket.close(); }

        /**
         *
         * @return Returns true if connection to server was restored, otherwise returns false
         */
        bool onConnectionDied();

        void scheduleDataSend(std::span<std::byte> buffer);

        bool onDataAvailable();
        void onDataSendingAvailable();

        /**
         * This function will start accepting packets
         * This function WILL yield
         */
        void run();
};
} // namespace Services
#endif