#ifndef SERVICEPROVIDER_H
#define SERVICEPROVIDER_H

#include "ServiceCollection.h"
#include "IService.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <typeindex>
#include <stdexcept>
#include <string>

// Forward declaration for dependency tracking
class ILoggingService;

/**
 * ServiceProvider - Resolution phase of DI container with validation
 * 
 * Inspired by .NET's IServiceProvider pattern with enhancements:
 * - Deterministic service resolution
 * - Dependency graph validation
 * - Circular dependency detection
 * - LoggingService-first enforcement
 * - Explicit lifecycle management
 * 
 * Build process:
 * 1. Validate LoggingService is first
 * 2. Resolve all services (construct in order)
 * 3. Validate dependency graph
 * 4. Configure all services
 * 
 * Lifecycle:
 * - StartServices(): Start in registration order
 * - StopServices(): Stop in reverse order (LoggingService last)
 */
class ServiceProvider {
public:
    /**
     * Exception thrown when Build() validation fails
     * Contains diagnostic information about the failure
     */
    class BuildException : public std::runtime_error {
    public:
        explicit BuildException(const std::string& message) : std::runtime_error(message) {}
    };

    /**
     * Construct ServiceProvider from ServiceCollection
     * This constructor does NOT build - call Build() separately
     * 
     * @param collection ServiceCollection with registered services
     */
    explicit ServiceProvider(const ServiceCollection& collection);

    /**
     * Destructor - stops all services in reverse order
     */
    ~ServiceProvider();

    /**
     * Build the service provider
     * Validates configuration, resolves services, and configures them
     * 
     * Validations performed:
     * - LoggingService must be registered and first
     * - No circular dependencies
     * - All dependencies resolvable
     * 
     * @return true if build succeeded, false otherwise
     * @throws BuildException if validation fails
     */
    bool Build();

    /**
     * Resolve a service by interface type
     * Returns the registered implementation (singleton)
     * 
     * @tparam TInterface Interface type to resolve
     * @return Shared pointer to implementation, or nullptr if not found
     */
    template<typename TInterface>
    std::shared_ptr<TInterface> Resolve() const {
        auto it = services_.find(std::type_index(typeid(TInterface)));
        if (it != services_.end()) {
            return std::static_pointer_cast<TInterface>(it->second);
        }
        return nullptr;
    }

    /**
     * Resolve a service by interface type (throws if not found)
     * 
     * @tparam TInterface Interface type to resolve
     * @return Shared pointer to implementation
     * @throws std::runtime_error if service not found
     */
    template<typename TInterface>
    std::shared_ptr<TInterface> GetRequiredService() const {
        auto service = Resolve<TInterface>();
        if (!service) {
            throw std::runtime_error("Service not found");
        }
        return service;
    }

    /**
     * Check if a service is registered
     * 
     * @tparam TInterface Interface type
     * @return true if service is registered, false otherwise
     */
    template<typename TInterface>
    bool IsRegistered() const {
        return services_.find(std::type_index(typeid(TInterface))) != services_.end();
    }

    /**
     * Get all services in construction order
     * Used for lifecycle management (Start/Stop)
     * 
     * @return Vector of services in construction order
     */
    const std::vector<std::shared_ptr<IService>>& GetAllServices() const {
        return servicesOrdered_;
    }

    /**
     * Configure all services
     * Calls Configure() on all services in registration order
     * Must be called after Build() succeeds
     */
    void ConfigureAll();

    /**
     * Start all services
     * Calls Start() on all services in registration order
     * LoggingService MUST be started first
     * Stops on first failure (but stops already-started services)
     * 
     * @return true if all services started successfully, false otherwise
     */
    bool StartServices();

    /**
     * Stop all services
     * Calls Stop() on all services in reverse order
     * LoggingService MUST be stopped last
     * Continues even if some services fail to stop
     * 
     * @return true if all services stopped successfully, false otherwise
     */
    bool StopServices();

    /**
     * Check if service provider is built
     * @return true if Build() was called successfully, false otherwise
     */
    bool IsBuilt() const { return isBuilt_; }

private:
    /**
     * Validate LoggingService is registered and first
     * Called during Build() validation
     * 
     * @return true if validation passes, false otherwise
     * @throws BuildException if validation fails
     */
    void ValidateLoggingServiceFirst();

    /**
     * Resolve all services from collection
     * Constructs services in registration order
     * LoggingService is constructed first (calls initLogging())
     * 
     * Note: This does NOT handle constructor injection
     * Constructor injection would require dependency tracking during construction
     */
    void ResolveAllServices();

    /**
     * Validate dependency graph
     * Detects circular dependencies and missing dependencies
     * 
     * Note: With current factory-based approach, dependency tracking
     * happens implicitly through construction order. This method
     * validates structural constraints (e.g., LoggingService first)
     */
    void ValidateDependencyGraph();

    /**
     * Check if LoggingService is first service
     * Structural validation - checks if first descriptor is ILoggingService
     * 
     * @return true if LoggingService is first, false otherwise
     */
    bool IsLoggingServiceFirst() const;

    /**
     * Services by interface type for type-safe resolution
     * All services are singletons (one instance per interface)
     */
    std::unordered_map<std::type_index, std::shared_ptr<IService>> services_;
    
    /**
     * Services in construction order for lifecycle management
     * Used for Start/Stop in correct order
     * LoggingService is first, stopped last
     */
    std::vector<std::shared_ptr<IService>> servicesOrdered_;

    /**
     * ServiceCollection reference (used during Build)
     * Stored to access descriptors during validation and resolution
     */
    const ServiceCollection& collection_;

    /**
     * Build state flag
     * Tracks whether Build() was called and succeeded
     */
    bool isBuilt_;
};

#endif // SERVICEPROVIDER_H
