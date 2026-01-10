#ifndef SERVICES_H
#define SERVICES_H

#include <memory>
#include <typeindex>
#include <unordered_map>

namespace Services {
class ServiceBase;

class ServiceProvider {
    private:
        std::unordered_map<std::type_index, std::unique_ptr<ServiceBase>> services;

    public:
        template <class T> T& getService() {
            auto servicesIt = services.find(typeid(T));
            if (servicesIt == services.end()) {
                servicesIt = services.emplace(typeid(T), std::make_unique<T>(*this)).first;
            }

            return *static_cast<T*>(servicesIt->second.get());
        }
};
} // namespace Services
#endif
