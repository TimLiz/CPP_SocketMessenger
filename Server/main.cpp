#include "Network/Server/Server.h"
#include "spdlog/spdlog.h"

int main() {
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_LEVEL_TRACE)); // Is being set by CMAKE

    Network::Server::Server server;
    server.run();
    return 0;
}
