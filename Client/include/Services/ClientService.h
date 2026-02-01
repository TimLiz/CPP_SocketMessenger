#ifndef MYMESSENGER_CLIENTSERVICE_H
#define MYMESSENGER_CLIENTSERVICE_H
#include "Epoll.h"
#include "Network/Socket.h"
#include "Services/ServiceBase.h"

#include "Network/PacketView.h"
#include "Network/Peer.h"
#include "list"

namespace Services {
class ClientService : public ServiceBase {
    private:
        std::unique_ptr<Network::Peer> networkPeer;
        std::shared_ptr<Epoll::Epoll> epoll = std::make_shared<Epoll::Epoll>();
        bool isRunning;

    public:
        ClientService(ServiceProvider& serviceProvider);

        int connect(std::string_view host, int port);

        /**
         *
         * @return Returns true if connection to server was restored, otherwise returns false
         */
        bool onConnectionDied();

        /**
         * This function will start accepting packets
         * This function WILL yield
         */
        void run();

        void stop();

        void schedulePacketSend(const flatbuffers::FlatBufferBuilder& builder) const;
};
} // namespace Services
#endif