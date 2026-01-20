#include "AppHost.h"
#include "DI/ServiceCollection.h"

/**
 * Main application entry point
 * 
 * Following .NET Generic Host / AppHost pattern:
 * - main() only wires and starts the application
 * - AppHost uses DI pattern: ConfigureServices() → Build() → Run()
 * - All initialization, run loop, and cleanup logic is in AppHost
 * 
 * Logging-first invariant: LoggingService constructor calls initLogging()
 * No output is produced before LoggingService is constructed
 */
int main() {
    // Create service collection and app host
    ServiceCollection services;
    AppHost host;
    
    // Configure services (registration only, no side effects)
    host.ConfigureServices(services);
    
    // Build ServiceProvider (constructs services, LoggingService calls initLogging())
    if (!host.Build(services)) {
        // Build failed - cannot output (logging may not be initialized)
        return -1;
    }
    
    // Run application lifecycle (Start → RunLoop → Stop)
    return host.Run();
}
