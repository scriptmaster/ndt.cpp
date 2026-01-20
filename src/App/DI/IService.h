#ifndef ISERVICE_H
#define ISERVICE_H

/**
 * IService - Base interface for all services
 * 
 * Defines the standard lifecycle contract for services:
 * - Configure(): Prepare service configuration (no IO, no threads, no side effects)
 * - Start(): Initialize service (real initialization - IO, threads, resources)
 * - Stop(): Cleanup service resources (reverse order of Start)
 * 
 * Following .NET's IService interface pattern
 */
class IService {
public:
    virtual ~IService() = default;

    /**
     * Configure the service
     * Called during Build() phase
     * Must not perform IO, create threads, or have side effects
     * Only prepares configuration settings
     */
    virtual void Configure() = 0;

    /**
     * Start the service
     * Called during Run() phase in registration order
     * Performs real initialization (IO, threads, resources)
     * 
     * @return true if start succeeded, false otherwise
     */
    virtual bool Start() = 0;

    /**
     * Stop the service
     * Called during cleanup in reverse order
     * Cleans up resources allocated in Start()
     */
    virtual void Stop() = 0;
};

#endif // ISERVICE_H
