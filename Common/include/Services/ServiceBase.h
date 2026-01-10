#ifndef SERVICEBASE_H
#define SERVICEBASE_H
#include "Services/ServiceProvider.h"
#include "boost/noncopyable.hpp"

namespace Services {
class ServiceBase : public boost::noncopyable {
    protected:
        ServiceProvider& _services;

        ServiceBase(ServiceProvider& serviceProvider) : _services(serviceProvider) {}

    public:
        virtual ~ServiceBase() = default;
};
} // namespace Services

#endif
