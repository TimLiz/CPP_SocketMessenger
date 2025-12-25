#ifndef SERVER_H
#define SERVER_H
#include <forward_list>
#include <unordered_set>
#include <sys/epoll.h>

#include "ClientConnection.h"
#include "../../../../Common/include/Epoll.h"
#include "../../../../Common/include/Network/Socket.h"

namespace Network::Server {
    class Server {
        friend class ClientConnection;

        private:
            Socket socket;
            Epoll::Epoll epoll{};

            void clientsProcessingThreadEntryPoint();

            void processClient(std::shared_ptr<Socket> clientSocket);

            void processClientDisconnect(std::shared_ptr<ClientConnection> connection);

        public:
            Server();
            ~Server();

            void run();

            std::unordered_map<int, std::shared_ptr<ClientConnection> > clients;
    };
}
#endif
