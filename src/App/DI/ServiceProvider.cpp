#include "ServiceProvider.h"
#include "../../Services/LoggingService/ILoggingService.h"
#include <typeinfo>
#include <iostream>
#include <sstream>

/**
 * ServiceProvider implementation
 * 
 * Resolution-phase DI container with validation and lifecycle management
 * Enforces LoggingService-first invariant and dependency graph validation
 */

ServiceProvider::ServiceProvider(const ServiceCollection& collection)
    : collection_(collection), isBuilt_(false) {
    // Constructor does NOT build - call Build() explicitly
    // This allows validation before any services are constructed
}

ServiceProvider::~ServiceProvider() {
    // Stop all services in reverse order
    // LoggingService is stopped last (it's first in servicesOrdered_)
    if (isBuilt_) {
        StopServices();
    }
}

bool ServiceProvider::Build() {
    /**
     * Build process:
     * 1. Validate LoggingService is first (structural validation)
     * 2. Resolve all services (construct in registration order)
     *    - LoggingService constructor calls initLogging()
     * 3. Validate dependency graph (structural checks)
     * 4. Configure all services
     */
    
    try {
        // STEP 1: Validate LoggingService is registered and first
        // This MUST happen before any service construction
        // Structural validation - no side effects
        ValidateLoggingServiceFirst();

        // STEP 2: Resolve all services
        // Services are constructed in registration order
        // LoggingService is constructed first (calls initLogging() in constructor)
        // After LoggingService, other services may use logging
        ResolveAllServices();

        // STEP 3: Validate dependency graph
        // Check for structural issues (e.g., LoggingService position)
        ValidateDependencyGraph();

        // STEP 4: Configure all services
        // Now logging is available (LoggingService constructed)
        std::cout << "[DEBUG] ServiceProvider::Build() - configuring all services..." << std::endl;
        ConfigureAll();

        isBuilt_ = true;
        std::cout << "[DEBUG] ServiceProvider::Build() - SUCCESS" << std::endl;
        return true;
    } catch (const BuildException& e) {
        // Build failed - cannot log if LoggingService validation failed
        // But we may be able to log if it failed later
        isBuilt_ = false;
        return false;
    } catch (...) {
        // Unexpected error during build
        isBuilt_ = false;
        return false;
    }
}

void ServiceProvider::ValidateLoggingServiceFirst() {
    /**
     * Validate LoggingService is registered and is the FIRST service
     * 
     * This is a structural validation - no services are constructed yet
     * We check the ServiceCollection descriptors directly
     * 
     * This validation is CRITICAL - if LoggingService is not first,
     * other services may attempt to log before logging is initialized
     */
    
    const auto& descriptors = collection_.GetDescriptors();
    
    if (descriptors.empty()) {
        throw BuildException("ServiceCollection is empty - LoggingService must be registered");
    }

    // Check if first descriptor is ILoggingService
    std::type_index loggingServiceType = std::type_index(typeid(ILoggingService));
    
    if (descriptors[0].interfaceType != loggingServiceType) {
        std::ostringstream oss;
        oss << "LoggingService MUST be registered FIRST in ServiceCollection. "
            << "Found " << descriptors.size() << " services, but first is not ILoggingService.";
        throw BuildException(oss.str());
    }

    // Validation passed - LoggingService is first
    // This ensures initLogging() will be called before any other service construction
}

void ServiceProvider::ResolveAllServices() {
    /**
     * Resolve all services from ServiceCollection
     * Services are constructed in registration order
     * 
     * CRITICAL: LoggingService is constructed first
     * LoggingService constructor calls initLogging()
     * After LoggingService is constructed, other services may use logging
     * 
     * All services are singletons - one instance per interface type
     */
    
    const auto& descriptors = collection_.GetDescriptors();
    
    for (const auto& descriptor : descriptors) {
        // Construct service using factory function
        // LoggingService constructor calls initLogging() immediately
        auto service = descriptor.factory();
        
        // Store by interface type for resolution (singleton)
        services_[descriptor.interfaceType] = service;
        
        // Store in construction order for lifecycle management
        servicesOrdered_.push_back(service);
    }
}

void ServiceProvider::ValidateDependencyGraph() {
    /**
     * Validate dependency graph structure
     * 
     * Current implementation uses factory-based construction (no explicit dependencies)
     * This method validates structural constraints:
     * - LoggingService is first (already validated in ValidateLoggingServiceFirst)
     * - All services are resolvable (they are - they're all in the collection)
     * 
     * Future enhancements for constructor injection:
     * - Track dependencies during resolution
     * - Detect circular dependencies
     * - Validate all dependencies are registered
     */
    
    // Structural validation: LoggingService must be first
    // This is already validated, but double-check after resolution
    if (!IsLoggingServiceFirst()) {
        throw BuildException("LoggingService is not first after resolution - build state invalid");
    }

    // All services should be resolvable (they are - constructed above)
    // Circular dependency detection would go here if constructor injection is added
}

bool ServiceProvider::IsLoggingServiceFirst() const {
    /**
     * Check if LoggingService is the first service in servicesOrdered_
     * 
     * @return true if LoggingService is first, false otherwise
     */
    
    if (servicesOrdered_.empty()) {
        return false;
    }

    // Check if first service is ILoggingService
    // Use dynamic_cast to check interface type
    auto firstService = servicesOrdered_[0];
    auto loggingService = std::dynamic_pointer_cast<ILoggingService>(firstService);
    
    return loggingService != nullptr;
}

void ServiceProvider::ConfigureAll() {
    /**
     * Configure all services in registration order
     * Called after services are constructed
     * 
     * LoggingService is already initialized (constructor called initLogging())
     * Other services may now use logging during Configure()
     */
    
    for (auto& service : servicesOrdered_) {
        try {
            service->Configure();
        } catch (...) {
            // Configuration failures are not fatal, but should be logged
            // After LoggingService is available, we can log here
            // For now, continue configuring other services
        }
    }
}

bool ServiceProvider::StartServices() {
    /**
     * Start all services in registration order
     * LoggingService MUST be started first
     * 
     * If any service fails to start:
     * - Stop all services that were started (in reverse order)
     * - Return false
     * 
     * @return true if all services started successfully, false otherwise
     */
    
    if (!isBuilt_) {
        // Cannot start if not built
        return false;
    }

    std::cout << "[DEBUG] ServiceProvider::StartServices() - starting all services..." << std::endl;

    // Track started services for rollback on failure
    std::vector<std::shared_ptr<IService>> started;

    // Start services in registration order
    // LoggingService is first (guaranteed by validation)
    for (auto& service : servicesOrdered_) {
        const char* serviceName = typeid(*service).name();
        try {
            std::cout << "[DEBUG] Starting service: " << serviceName << std::endl;
            if (!service->Start()) {
                // Start failed - rollback all started services
                std::cout << "[ERROR] Service failed to start - rolling back... (" << serviceName << ")" << std::endl;
                for (auto it = started.rbegin(); it != started.rend(); ++it) {
                    try {
                        (*it)->Stop();
                    } catch (...) {
                        // Continue stopping even if some fail
                    }
                }
                return false;
            }
            started.push_back(service);
        } catch (const std::exception& e) {
            // Exception during start - rollback all started services
            std::cout << "[ERROR] Exception during service start - rolling back... (" << serviceName
                      << ") " << e.what() << std::endl;
            for (auto it = started.rbegin(); it != started.rend(); ++it) {
                try {
                    (*it)->Stop();
                } catch (...) {
                    // Continue stopping even if some fail
                }
            }
            return false;
        } catch (...) {
            // Exception during start - rollback all started services
            std::cout << "[ERROR] Exception during service start - rolling back... (" << serviceName << ")" << std::endl;
            for (auto it = started.rbegin(); it != started.rend(); ++it) {
                try {
                    (*it)->Stop();
                } catch (...) {
                    // Continue stopping even if some fail
                }
            }
            return false;
        }
    }
    
    std::cout << "[DEBUG] ServiceProvider::StartServices() - SUCCESS" << std::endl;
    return true;
}

bool ServiceProvider::StopServices() {
    /**
     * Stop all services in reverse order
     * LoggingService MUST be stopped last (it's first in servicesOrdered_)
     * 
     * Continues even if some services fail to stop
     * This ensures all cleanup is attempted
     * 
     * @return true if all services stopped successfully, false otherwise
     */
    
    if (!isBuilt_) {
        // Nothing to stop if not built
        return true;
    }

    std::cout << "[DEBUG] ServiceProvider::StopServices() - stopping all services..." << std::endl;
    
    // Stop services in reverse order
    // LoggingService is stopped last (it's first in servicesOrdered_, so last in reverse)
    bool allStopped = true;
    
    for (auto it = servicesOrdered_.rbegin(); it != servicesOrdered_.rend(); ++it) {
        const char* serviceName = typeid(**it).name();
        std::cout << "[DEBUG] Stopping service: " << serviceName << std::endl;
        try {
            (*it)->Stop();
            std::cout << "[DEBUG] Service stopped: " << serviceName << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] Service stop failed: " << serviceName << std::endl;
            // Continue stopping other services even if one fails
            allStopped = false;
        }
    }
    
    std::cout << "[DEBUG] ServiceProvider::StopServices() - " 
              << (allStopped ? "SUCCESS" : "completed with errors") << std::endl;
    return allStopped;
}
