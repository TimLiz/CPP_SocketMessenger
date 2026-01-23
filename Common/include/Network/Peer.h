#ifndef MYMESSENGER_PEER_H
#define MYMESSENGER_PEER_H
#include "Epoll.h"
#include "Socket.h"

namespace Network {

class Peer {
    private:
        Socket socket;

    protected:
        virtual int modifyEpoll(int fd, epoll_event& event) = 0;

    public:
};

} // namespace Network

#endif
