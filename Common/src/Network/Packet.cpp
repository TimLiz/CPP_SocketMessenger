#include "../../include/Network/Packet.h"
#include "fbs/Base_generated.h"

namespace Network {
    Packet::Packet(std::span<const std::byte> dataView) {
        this->buffer = std::vector(dataView.begin(), dataView.end());
    }

    std::unique_ptr<const Packets::Base> Packet::GetParsedView() {
        if (!verified) {
            throw std::logic_error("Tried to parse non verified packet");
        }
        return std::unique_ptr<const Packets::Base>(Packets::GetBase(this->buffer.data()));
    }

    bool Packet::verify() {
        auto verifier = flatbuffers::Verifier(reinterpret_cast<unsigned char*>(this->buffer.data()), this->buffer.size());
        verified = Packets::VerifyBaseBuffer(verifier);
        return verified;
    }
}
