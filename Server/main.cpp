#include "spdlog/spdlog.h"

#include "Services/PacketsDispatchService.h"
#include "Services/ServerService.h"
#include "Services/ServiceProvider.h"

int main() {
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL)); // Is being set by CMAKE
    Services::ServiceProvider serviceProvider;

    serviceProvider.getService<Services::PacketsDispatchService>().subscribeToPacket(
        Packets::Packets_ClientHello,
        [](const Network::Packets::Base* packet) { SPDLOG_CRITICAL("Hello from Client"); });

    serviceProvider.getService<Services::ServerService>().run();
    return 0;
}
