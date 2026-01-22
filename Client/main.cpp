#include <iostream>

#include "Network/Socket.h"
#include "Services/ClientService.h"
#include "fbs/Base_generated.h"
#include "fbs/ClientHello_generated.h"
#include "spdlog/spdlog.h"

#include "Services/PacketsDispatchService.h"

int main() {
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL)); // Is being set by CMAKE

    Services::ServiceProvider service_provider;
    service_provider.getService<Services::PacketsDispatchService>().subscribeToPacket(
        Network::Packets::Packets::Packets_ServerHello,
        [](auto packet, auto ctx) { SPDLOG_INFO("Hello from server!"); });

    auto& clientService = service_provider.getService<Services::ClientService>();
    if (clientService.connect("127.0.0.1", 25365) == -1) {
        spdlog::critical("Failed to connect to server");
        return -1;
    }

    auto fb_builder = flatbuffers::FlatBufferBuilder();
    auto clientHelloPacket = Network::Packets::CreateClientHello(fb_builder);
    auto basePacket =
        Network::Packets::CreateBase(fb_builder, Network::Packets::Packets_ClientHello, clientHelloPacket.Union());
    fb_builder.Finish(basePacket);

    clientService.scheduleDataSend({reinterpret_cast<std::byte*>(fb_builder.GetBufferPointer()), fb_builder.GetSize()});

    clientService.run();
}