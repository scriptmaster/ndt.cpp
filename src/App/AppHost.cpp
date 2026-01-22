#include "AppHost.h"
#include "../Services/LoggingService/ILoggingService.h"
#include "../Services/LoggingService/LoggingService.h"
#include "../Services/LocalConfigService/ILocalConfigService.h"
#include "../Services/LocalConfigService/LocalConfigService.h"
#include "../Services/WindowService/IWindowService.h"
#include "../Services/WindowService/WindowService.h"
#include "../Services/WindowService/TestWindowService.h"
#include "../Services/AudioPlayerService/IAudioPlayerService.h"
#include "../Services/AudioPlayerService/AudioPlayerService.h"
#include "../Services/NetworkService/INetworkService.h"
#include "../Services/NetworkService/NetworkService.h"
#include "../Services/AudioProcessorService/IAudioProcessorService.h"
#include "../Services/AudioProcessorService/AudioProcessorService.h"
#include "../Services/AudioCaptureService/IAudioCaptureService.h"
#include "../Services/AudioCaptureService/AudioCaptureService.h"
#include "../Services/HTTPService/IHTTPService.h"
#include "../Services/HTTPService/HTTPService.h"
#include "../Services/WSService/IWSService.h"
#include "../Services/WSService/WSService.h"
#include "../Services/STTService/ISTTService.h"
#include "../Services/STTService/STTService.h"
#include "../Services/LLMService/ILLMService.h"
#include "../Services/LLMService/LLMService.h"
#include "../Services/TTSService/ITTSService.h"
#include "../Services/TTSService/TTSService.h"
#include <cstdlib>
#include <iostream>
#include <string>

/**
 * AppHost implementation
 * Manages complete application lifecycle using DI pattern
 * Enforces logging-first invariant via LoggingService constructor
 */

AppHost::AppHost() {
}

AppHost::~AppHost() {
    // Destructor is passive - cleanup is handled explicitly by Run()
}

void AppHost::ConfigureServices(ServiceCollection& services) {
    /**
     * Register all services
     * LoggingService MUST be registered FIRST
     * LoggingService constructor calls initLogging() immediately
     * Other services may only output after LoggingService is constructed
     */
    
    // STEP 1: Register LoggingService FIRST (MANDATORY)
    // LoggingService constructor will call initLogging() when instantiated
    services.Register<ILoggingService, LoggingService>();
    
    // STEP 2: Register LocalConfigService (loads config files)
    services.Register<ILocalConfigService, LocalConfigService>();
    
    // STEP 3: Register WindowService (test mode uses TestWindowService)
    const char* envMode = std::getenv("ENV");
    bool isTestMode = envMode && std::string(envMode) == "test";
    if (isTestMode) {
        services.Register<IWindowService, TestWindowService>();
    } else {
        services.Register<IWindowService, WindowService>();
    }
    
    // STEP 4: Register AudioPlayerService (audio seed + waveform)
    services.Register<IAudioPlayerService, AudioPlayerService>();
    
    // STEP 5: Register NetworkService (network init before capture)
    services.Register<INetworkService, NetworkService>();
    
    // STEP 6: Register AudioCaptureService (microphone capture)
    services.Register<IAudioCaptureService, AudioCaptureService>();
    
    // STEP 7: Register AudioProcessorService (speech chunking)
    services.Register<IAudioProcessorService, AudioProcessorService>();
    
    // STEP 8: Register stub services (future functionality)
    services.Register<IHTTPService, HTTPService>();
    services.Register<IWSService, WSService>();
    services.Register<ISTTService, STTService>();
    services.Register<ILLMService, LLMService>();
    services.Register<ITTSService, TTSService>();
    // services.Register<W2LSService, W2LService>();
}

bool AppHost::Build(ServiceCollection& services) {
    /**
     * Build ServiceProvider from ServiceCollection
     * 
     * Build process:
     * 1. Create ServiceProvider (does not build yet - allows validation)
     * 2. Call Build() - validates, constructs services, configures them
     *    - Validates LoggingService is first
     *    - Constructs services (LoggingService calls initLogging() first)
     *    - Validates dependency graph
     *    - Configures all services
     */
    
    try {
        // Create ServiceProvider - this does NOT build yet
        // This allows validation before any services are constructed
        serviceProvider_ = std::make_shared<ServiceProvider>(services);
        
        // Build ServiceProvider - this validates and constructs services
        // LoggingService constructor will call initLogging() during construction
        if (!serviceProvider_->Build()) {
            // Build failed - validation or construction failed
            // Cannot output here if LoggingService validation failed
            return false;
        }
        
        // Now logging is active - LoggingService was constructed and initLogging() called
        std::cout << "[DEBUG] Starting AppHost::Build()..." << std::endl;
        std::cout << "[DEBUG] STEP 1: Logging initialized - SUCCESS" << std::endl;
        std::cout << "[DEBUG] STEP 1: All services configured - SUCCESS" << std::endl;
        
        return true;
    } catch (const ServiceProvider::BuildException& e) {
        // Build validation failed - cannot output if LoggingService validation failed
        return false;
    } catch (...) {
        // Unexpected error during build
        return false;
    }
}

int AppHost::Run() {
    /**
     * Run the complete application lifecycle
     * Owns the full lifecycle: Start services → RunLoop → Stop services
     * 
     * Services are started in registration order
     * Main loop runs via WindowService
     * Services are stopped in reverse order
     */
    
    if (!serviceProvider_) {
        // ServiceProvider not built - cannot run
        // Cannot output - logging may not be initialized
        return -1;
    }
    
    // STEP 2: Start all services in registration order
    std::cout << "[DEBUG] Starting all services..." << std::endl;
    if (!serviceProvider_->StartServices()) {
        std::cerr << "[ERROR] Failed to start all services" << std::endl;
        return -1;
    }

    // STEP 3: Resolve WindowService for main loop
    auto windowService = serviceProvider_->Resolve<IWindowService>();
    
    std::cout << "[DEBUG] STEP 4: Displaying startup messages..." << std::endl;
    std::cout << "Display Running..." << std::endl;
    std::cout << "Press ESC, Alt+F4, or close windows to exit" << std::endl;
    std::cout << "[DEBUG] STEP 4: Startup messages displayed - SUCCESS" << std::endl;
    
    // STEP 5: Run main loop via WindowService
    int runResult = 0;
    if (windowService) {
        runResult = windowService->RunLoop();
    } else {
        std::cerr << "[ERROR] WindowService not available" << std::endl;
        runResult = -1;
    }

    if (windowService) {
        std::cout << "[DEBUG] Explicitly stopping WindowService..." << std::endl;
        windowService->Stop();
        std::cout << "[DEBUG] WindowService stopped via AppHost" << std::endl;
    }

    // STEP 7: Stop all services in reverse order
    // ServiceProvider destructor will call StopServices() on all services
    // But we call explicitly for proper error handling
    serviceProvider_->StopServices();
    
    std::cout << "[DEBUG] AppHost::Run() completed" << std::endl;
    return runResult;
}

