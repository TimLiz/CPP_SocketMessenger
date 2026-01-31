#ifndef MYMESSENGER_PEER_H
#define MYMESSENGER_PEER_H
#include "Epoll.h"
#include "Socket.h"

#include "list"

#include "Transport/BasicTransport.h"

namespace Network {

class Peer {
    private:
        bool isEnabled = false;

        using PacketHandlerCallable = std::function<void(const Packets::Base* packet)>;

        std::unique_ptr<ITransport> transport;

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

        void trySyncEpollInterests() const;

    public:
        bool isConnected = true;

        Peer(const std::shared_ptr<Epoll::Epoll>& epl, PacketHandlerCallable packetHandler);

        virtual ~Peer();

        void scheduleBufferSend(std::span<const std::byte> buffer) const;

        int getFd() const noexcept {
            if (transport == nullptr) {
                return -1;
            }
            return transport->getFd();
        }

        void setTransport(std::unique_ptr<ITransport> newTransport);

        void setEpollData(const epoll_data data) noexcept { epollExtraData = data; }

        /**
         * Needs to be called to add initial interests into epoll
         */
        void enable();

        /**
         * Must be called each time incoming data detected by epoll controller
         * @return False if remote system disconnected
         */
        bool onDataAvailable();

        void onDataSendingAvailable() const;

        void disconnect();
};

} // namespace Network

#endif
