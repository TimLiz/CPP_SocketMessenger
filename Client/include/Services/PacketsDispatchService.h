#ifndef MYMESSENGER_PACKETSDISPATCHSERVICE_H
#define MYMESSENGER_PACKETSDISPATCHSERVICE_H
#include "PacketsDispatchService_/ClientPacketContext.h"
#include "Services/PacketsDispatchService/PacketsDispatchService.h"

namespace Services {
typedef PacketsDispatchService_::PacketsDispatchService_<PacketsDispatchService_::ClientPacketContext>
    PacketsDispatchService; // Alias for easier access
}

#endif
