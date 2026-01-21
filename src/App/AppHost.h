#ifndef APPHOST_H
#define APPHOST_H

#include "DI/ServiceCollection.h"
#include "DI/ServiceProvider.h"
#include <memory>

// Forward declarations
class ILoggingService;
class ILocalConfigService;
class IWindowService;
class IAudioPlayerService;
class INetworkService;
class IAudioProcessorService;
class IAudioCaptureService;

/**
 * AppHost - Application Host
 * Inspired by .NET's Generic Host / IHost pattern
 * 
 * Encapsulates the complete application lifecycle using DI:
 * - ConfigureServices(): Register all services
 * - Build(): Create ServiceProvider and configure services
 * - Run(): Start services → RunLoop → Stop services
 * 
 * Logging-first invariant: LoggingService MUST be registered FIRST
 * LoggingService constructor calls initLogging() before any other output
 */
class AppHost {
public:
    /**
     * Construct AppHost
     * Constructor produces no output (logging not initialized yet)
     */
    AppHost();

    /**
     * Destructor - passive cleanup only
     * Cleanup is handled explicitly by Run()
     */
    ~AppHost();

    /**
     * Configure services (registration phase)
     * Registers all services with their interfaces and implementations
     * LoggingService MUST be registered FIRST
     * 
     * No side effects - only registration
     */
    void ConfigureServices(ServiceCollection& services);

    /**
     * Build ServiceProvider from ServiceCollection
     * Creates all service instances and calls Configure() on them
     * Services are constructed in registration order
     * 
     * @return true if build succeeded, false otherwise
     */
    bool Build(ServiceCollection& services);

    /**
     * Run the complete application lifecycle
     * Owns the full lifecycle: Start services → RunLoop → Stop services
     * 
     * @return Exit code (0 for success, non-zero for errors)
     */
    int Run();

    /**
     * Get ServiceProvider for service resolution
     * @return Shared pointer to ServiceProvider, or nullptr if not built
     */
    std::shared_ptr<ServiceProvider> GetServiceProvider() const {
        return serviceProvider_;
    }

private:
    /**
     * ServiceProvider instance
     * Created during Build(), used during Run()
     */
    std::shared_ptr<ServiceProvider> serviceProvider_;
};

#endif // APPHOST_H
