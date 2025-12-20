#ifndef PACKET_H
#define PACKET_H
#include <cstddef>
#include <span>
#include <vector>

#include "PacketHeader.h"

namespace Network {
class Packet {
    private:
        std::vector<std::byte> buffer;
        bool verified = false;
    public:
        Packet(std::span<const std::byte> dataView);
        bool verify();
        std::unique_ptr<const Packets::Base> GetParsedView();
};
}
#endif
