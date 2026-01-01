#ifndef SERVICEBASE_H
#define SERVICEBASE_H
#include "boost/noncopyable.hpp"
#include "Network/Dispatcher.h"

namespace Services {
    struct Services;

    class ServiceBase : boost::noncopyable {
        protected:
            Services& _services;

        public:
            ServiceBase::ServiceBase(Services& services): _services(services) {}

            virtual ~ServiceBase() = 0;

            virtual void networkInit(Network::Dispatcher& dispatcher) {};
    };
}

#endif
