#ifndef PACKET_H
#define PACKET_H
#include <cstddef>
#include <span>
#include <vector>

#include "PacketHeader.h"

namespace Network {
class PacketView {
    private:
        const std::span<std::byte> buffer;
        bool verified = false;

    public:
        using PACKET_SIZE_TYPE = unsigned int;

        static constexpr PACKET_SIZE_TYPE MAXIMUM_PACKET_SIZE = 1'000'000;

        PacketView(const std::span<std::byte>& dataView);

        bool verify();

        const Packets::Base* GetParsedView();
        std::span<const std::byte> getBufferView();
};
} // namespace Network
#endif
