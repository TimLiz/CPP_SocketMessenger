#ifndef MYMESSENGER_PACKETSDISPATCHSERVICE_H
#define MYMESSENGER_PACKETSDISPATCHSERVICE_H
#include "PacketsDispatchService_/ServerPacketContext.h"
#include "Services/PacketsDispatchService/PacketsDispatchService.h"

namespace Services {
typedef PacketsDispatchService_::PacketsDispatchService_<PacketsDispatchService_::ServerPacketContext>
    PacketsDispatchService; // Alias for easier access
}

#endif
