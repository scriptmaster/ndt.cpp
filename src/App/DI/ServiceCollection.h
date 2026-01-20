#ifndef SERVICECOLLECTION_H
#define SERVICECOLLECTION_H

#include "IService.h"
#include <memory>
#include <vector>
#include <functional>
#include <typeindex>

/**
 * ServiceCollection - Registration phase of DI container
 * 
 * Inspired by .NET's IServiceCollection pattern
 * Holds service registrations (interface -> implementation factories)
 * Maintains registration order for proper lifecycle management
 */
class ServiceCollection {
public:
    /**
     * Service registration descriptor
     * Stores interface type and factory function for creating implementation
     */
    struct ServiceDescriptor {
        std::type_index interfaceType;
        std::function<std::shared_ptr<IService>()> factory;
    };

    /**
     * Register a service with interface and implementation
     * Factory function will be called during Build() to create instance
     * 
     * @tparam TInterface Interface type
     * @tparam TImplementation Implementation type
     */
    template<typename TInterface, typename TImplementation>
    void Register() {
        descriptors_.push_back({
            std::type_index(typeid(TInterface)),
            []() -> std::shared_ptr<IService> {
                return std::make_shared<TImplementation>();
            }
        });
    }

    /**
     * Register a service with factory function
     * Allows custom construction logic (e.g., constructor injection)
     * 
     * @tparam TInterface Interface type
     * @param factory Factory function that creates the implementation
     */
    template<typename TInterface>
    void Register(std::function<std::shared_ptr<IService>()> factory) {
        descriptors_.push_back({
            std::type_index(typeid(TInterface)),
            factory
        });
    }

    /**
     * Get all service descriptors
     * Used by ServiceProvider during Build() to resolve services
     * 
     * @return Vector of service descriptors in registration order
     */
    const std::vector<ServiceDescriptor>& GetDescriptors() const {
        return descriptors_;
    }

    /**
     * Clear all registrations
     * Useful for testing or reconfiguration
     */
    void Clear() {
        descriptors_.clear();
    }

private:
    // Service descriptors in registration order
    std::vector<ServiceDescriptor> descriptors_;
};

#endif // SERVICECOLLECTION_H
