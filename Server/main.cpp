#include "spdlog/spdlog.h"

#include "Services/Services.h"

int main() {
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL)); // Is being set by CMAKE

    Services::Services services;
    services.getService<Services::ServerService>()->run();
    //
    // Network::Server::Server server;
    // server.run();
    return 0;
}
