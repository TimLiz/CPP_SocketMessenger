#ifndef MYMESSENGER_SECUREPEER_H
#define MYMESSENGER_SECUREPEER_H

#include "Peer.h"

namespace Network {

class SecurePeer : public Peer {
    public:
        SecurePeer(const std::shared_ptr<Epoll::Epoll>& epl, const PacketHandlerCallable& packetHandler)
            : Peer(epl, packetHandler) {}
};

} // namespace Network

#endif