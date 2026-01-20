#include <iostream>

#include "../Common/include/Network/Socket.h"
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

    Network::Socket clientSocket(Network::SocketType::SOCK_STREAM);
    if (clientSocket.connect("127.0.0.1", 25365) == -1) {
        spdlog::critical("Failed to connect to server");
        return -1;
    }

    flatbuffers::FlatBufferBuilder builder;
    auto clientHelloPacket = Network::Packets::CreateClientHello(builder);
    auto basePacket =
        Network::Packets::CreateBase(builder, Network::Packets::Packets_ClientHello, clientHelloPacket.Union());
    builder.Finish(basePacket);

    std::vector<std::byte> sendBuffer;
    sendBuffer.resize(4 + builder.GetSize());
    sendBuffer[0] = static_cast<std::byte>(builder.GetSize());
    std::memcpy(sendBuffer.data() + 4, builder.GetBufferPointer(), builder.GetSize());

    std::vector<iovec> iov;
    iov.push_back({sendBuffer.data(), sendBuffer.size()});

    if (clientSocket.send(iov) == -1) {
        spdlog::critical("Failed to send client hello");
    }

    int e;
    std::cin >> e;
    return 0;
}