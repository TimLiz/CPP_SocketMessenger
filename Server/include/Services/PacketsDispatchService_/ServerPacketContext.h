#ifndef MYMESSENGER_SERVERPACKETCONTEXT_H
#define MYMESSENGER_SERVERPACKETCONTEXT_H

namespace Network::Server {
class ClientConnection;
}

namespace Services::PacketsDispatchService_ {

struct ServerPacketContext {
    public:
        Network::Server::ClientConnection& clientConnection;
};
} // namespace Services::PacketsDispatchService_

#endif
