#ifndef SERVICEBASE_H
#define SERVICEBASE_H
#include "boost/noncopyable.hpp"
#include "Network/Dispatcher.h"
#include "spdlog/spdlog.h"

namespace Services {
    struct Services;

    class ServiceBase : boost::noncopyable {
        protected:
            Services* _services;

            ServiceBase(Services* services): _services(services) {}
        public:
            virtual ~ServiceBase() = default;
    };
}

#endif
