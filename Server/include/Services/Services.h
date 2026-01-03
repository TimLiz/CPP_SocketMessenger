#ifndef SERVICES_H
#define SERVICES_H

#include <any>
#include <memory>
#include <typeindex>
#include <unordered_map>

#include "ServerService.h"

namespace Services {
    class Services {
        private:
            std::unordered_map<std::type_index, std::shared_ptr<void>> services;
        public:
            template<class T>
            std::shared_ptr<T> getService() {
                auto servicesIt = services.find(typeid(T));
                if (servicesIt == services.end()) {
                    // servicesIt = services.emplace(typeid(T), std::make_shared<T>(this)).first;
                    servicesIt = services.emplace(typeid(T), std::make_shared<T>(this)).first;
                }

                return static_pointer_cast<T>(servicesIt->second);
            }
    };
}
#endif
