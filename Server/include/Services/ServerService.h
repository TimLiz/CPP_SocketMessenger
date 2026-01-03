#ifndef SERVERSERVICE_H
#define SERVERSERVICE_H
#include "ServiceBase.h"
#include "spdlog/spdlog.h"
#include "Network/Socket.h"
#include "Network/Server/ClientConnection.h"
#include "Epoll.h"

using namespace Network;

namespace Services {
    class ServerService : public ServiceBase {
        friend class Server::ClientConnection;

        private:
            Socket socket;
            Epoll::Epoll epoll{};

            void clientsProcessingThreadEntryPoint();

            void processClient(const std::shared_ptr<Socket>& clientSocket);

            void processClientDisconnect(std::shared_ptr<Server::ClientConnection> connection);

        public:
            ServerService(Services* services);

            void run();


            std::unordered_map<int, std::shared_ptr<Server::ClientConnection>> clients;
    };
}
#endif
