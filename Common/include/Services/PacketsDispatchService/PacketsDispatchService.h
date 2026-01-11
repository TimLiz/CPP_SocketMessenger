#ifndef MYMESSENGER_DO_NOT_USE_PACKETSDISPATCHSERVICE_H
#define MYMESSENGER_DO_NOT_USE_PACKETSDISPATCHSERVICE_H
#include "Services/ServiceBase.h"
#include "fbs/Base_generated.h"

namespace Services::PacketsDispatchService_ {

struct ServerPacketContext;
struct ClientPacketContext;

template <typename T>
concept PacketDispatchContext = std::same_as<T, ServerPacketContext> || std::same_as<T, ClientPacketContext>;

/**
 * To avoid typing long type use alias Services::PacketsDispatchService
 * Alias is defined with predefined ContextType in /Services/PacketDispatchService.h on Client and Server builds
 * @tparam ContextType The context type, might be either PacketsDispatchService::ServerContext or
 * PacketsDispatchService::ClientContext
 */
template <PacketDispatchContext ContextType> class PacketsDispatchService_ : public ServiceBase {
    private:
        typedef std::function<void(const Network::Packets::Base* packet, const ContextType& context)>
            subscriberFunction;
        std::unordered_map<Network::Packets::Packets, std::vector<subscriberFunction>> subscribers;

    public:
        PacketsDispatchService_(ServiceProvider& serviceProvider) : ServiceBase(serviceProvider) {}

        void subscribeToPacket(Network::Packets::Packets packet, const subscriberFunction& subscriber) {
            subscribers[packet].push_back(subscriber);
        }

        void dispatchPacket(const Network::Packets::Base* packet, const ContextType& context) {
            for (auto& subscriber : subscribers[packet->packet_union_type()]) {
                subscriber(packet, context);
            }
        }
};
} // namespace Services::PacketsDispatchService_

#endif
