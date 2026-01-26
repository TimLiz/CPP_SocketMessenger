#ifndef MYMESSENGER_PEER_H
#define MYMESSENGER_PEER_H
#include "Epoll.h"
#include "Socket.h"

#include "list"

namespace Network {

class Peer {
        using PacketHandlerCallable = std::function<void(const Packets::Base* packet)>;

    private:
        std::unique_ptr<Socket> socket;
        std::shared_ptr<Epoll::Epoll> epoll;

        epoll_data epollExtraData;

        /**
         * This is list of buffers queued for send to the remote system
         */
        std::list<std::vector<std::byte>> sendBuffers;

        // The size of currently processing packet announced by client
        PacketView::PACKET_SIZE_TYPE currentPacketSizeExpected = 0;
        int bytesInReadingBuffer = 0; // Amount of bytes in reading buffer
        std::array<std::byte, 16000> rBuffer{};
        std::vector<std::byte> packetBuffer; // Stores all chunks of current packet

        PacketHandlerCallable packetHandler;

    public:
        bool isConnected = true;

        Peer(const std::shared_ptr<Epoll::Epoll>& epl, PacketHandlerCallable packetHandler);

        virtual ~Peer();

        void scheduleBufferSend(std::span<const std::byte> buffer);

        const int getFd() const noexcept { return socket->getFd(); }

        void setSocket(std::unique_ptr<Socket> newSocket);

        void setEpollData(const epoll_data data) noexcept { epollExtraData = data; }

        /**
         * Must be called each time incoming data detected by epoll controller
         * @return False if remote system disconnected
         */
        bool onDataAvailable();

        void onDataSendingAvailable();

        void disconnect();
};

} // namespace Network

#endif
