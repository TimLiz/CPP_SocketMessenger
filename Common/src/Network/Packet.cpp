#include "../../include/Network/Packet.h"
#include "fbs/Base_generated.h"

namespace Network {
    Packet::Packet(const std::span<const std::byte> dataView) {
        this->buffer.resize(dataView.size() + sizeof(PACKET_SIZE_TYPE));
        constexpr int PACKET_SIZE_TYPE_SIZE = sizeof(PACKET_SIZE_TYPE);

        std::memcpy(this->buffer.data(), &PACKET_SIZE_TYPE_SIZE, sizeof(PACKET_SIZE_TYPE_SIZE));
        std::memcpy(this->buffer.data() + PACKET_SIZE_TYPE_SIZE, dataView.data(), dataView.size());
    }

    std::unique_ptr<const Packets::Base> Packet::GetParsedView() {
        if (!verified) {
            throw std::logic_error("Tried to parse non verified packet");
        }
        return std::unique_ptr<const Packets::Base>(Packets::GetBase(this->buffer.data()));
    }

    std::span<const std::byte> Packet::getBufferView() {
        return this->buffer;
    }

    bool Packet::verify() {
        auto verifier = flatbuffers::Verifier(reinterpret_cast<unsigned char*>(this->buffer.data()), this->buffer.size());
        verified = Packets::VerifyBaseBuffer(verifier);
        return verified;
    }
}
