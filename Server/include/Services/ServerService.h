#ifndef SERVERSERVICE_H
#define SERVERSERVICE_H
#include "Epoll.h"
#include "Network/Server/ClientConnection.h"
#include "Network/Socket.h"
#include "Services/ServiceBase.h"
#include "spdlog/spdlog.h"

namespace Services {
class ServerService : public ServiceBase {
        friend class Network::Server::ClientConnection;

    private:
        Network::Socket socket;
        std::shared_ptr<Epoll::Epoll> epoll = std::make_shared<Epoll::Epoll>();

        void clientsProcessingThreadEntryPoint();

        void processClient(std::unique_ptr<Network::Socket> clientSocket);

        void processClientDisconnect(std::shared_ptr<Network::Server::ClientConnection> connection);

    public:
        ServerService(ServiceProvider& serviceProvider);

        void run();

        std::unordered_map<int, std::shared_ptr<Network::Server::ClientConnection>> clients;
};
} // namespace Services
#endif
