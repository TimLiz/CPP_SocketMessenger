#ifndef SERVERSERVICE_H
#define SERVERSERVICE_H
#include "Epoll.h"
#include "Network/Server/ClientConnection.h"
#include "Network/Socket.h"
#include "Services/ServiceBase.h"
#include "spdlog/spdlog.h"

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
            ServerService(ServiceProvider& serviceProvider);

            void run();

            std::unordered_map<int, std::shared_ptr<Server::ClientConnection>> clients;
    };
} // namespace Services
#endif
