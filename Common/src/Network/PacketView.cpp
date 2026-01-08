#include "../../include/Network/PacketView.h"
#include "fbs/Base_generated.h"

namespace Network {
    PacketView::PacketView(const std::span<std::byte>& dataView): buffer(dataView) {}

    const Packets::Base* PacketView::GetParsedView() {
        if (!verified) {
            throw std::logic_error("Tried to parse non verified packet");
        }
        return Packets::GetBase(this->buffer.data());
    }

    std::span<const std::byte> PacketView::getBufferView() {
        return this->buffer;
    }

    bool PacketView::verify() {
        auto verifier = flatbuffers::Verifier(reinterpret_cast<unsigned char*>(this->buffer.data()),
                                              this->buffer.size());
        verified = Packets::VerifyBaseBuffer(verifier);
        return verified;
    }
}
