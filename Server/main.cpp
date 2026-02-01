#include "spdlog/spdlog.h"

#include "Services/PacketsDispatchService.h"
#include "Services/ServerService.h"
#include "Services/ServiceProvider.h"

#include "fbs/ServerHello_generated.h"

int main() {
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL)); // Is being set by CMAKE
    Services::ServiceProvider serviceProvider;

    serviceProvider.getService<Services::PacketsDispatchService>().subscribeToPacket(
        Network::Packets::Packets_ClientHello, [](auto packet, auto context) {
            SPDLOG_INFO("ClientHello from connection #{}", context.clientConnection.connectionId);

            auto fb_builder = flatbuffers::FlatBufferBuilder();
            auto serverHelloPacket = Network::Packets::CreateServerHello(fb_builder);
            auto basePacket = Network::Packets::CreateBase(fb_builder, Network::Packets::Packets_ServerHello,
                                                           serverHelloPacket.Union());
            fb_builder.Finish(basePacket);

            context.clientConnection.schedulePacketSend(fb_builder);

            fb_builder.Clear();
        });

    serviceProvider.getService<Services::ServerService>().run();
    return 0;
}
