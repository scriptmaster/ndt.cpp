#include "AppHost.h"
#include "DI/ServiceCollection.h"
#ifdef _WIN32
#include <windows.h>
#endif

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
int app_main() {
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
    host.Run();

    // std::cout << "Hello, World!" << std::endl;
    return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return app_main();
}
#else
int main() {
    return app_main();
}
#endif
