#ifndef SERVER_H
#define SERVER_H
#include <forward_list>
#include <unordered_set>
#include <sys/epoll.h>

#include "ClientConnection.h"

namespace Network::Server {
    class Server {
        private:
            int socketId = -1;
            int clientConnectionsEpoll;

            void clientsProcessingThreadEntryPoint();
        public:
            Server();
            ~Server();

            std::unordered_map<int, std::unique_ptr<ClientConnection>> serverConnections;
            std::unordered_set<int> clientConnectionsToDisconnect;
            void setEpollEventForConnection(int connectionId, epoll_event &event) const;
    };
}
#endif
