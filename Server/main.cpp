#include "spdlog/spdlog.h"

#include "Services/PacketsDispatchService.h"
#include "Services/ServerService.h"
#include "Services/ServiceProvider.h"
int main() {
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL)); // Is being set by CMAKE
    Services::ServiceProvider serviceProvider;

    serviceProvider.getService<Services::PacketsDispatchService>().subscribeToPacket(
        Network::Packets::Packets_ClientHello, [](auto packet, auto context) {
            SPDLOG_INFO("ClientHello from connection #{}", context.clientConnection.connectionId);
        });

    serviceProvider.getService<Services::ServerService>().run();
    return 0;
}
