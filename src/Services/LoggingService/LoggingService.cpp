#include "LoggingService.h"
#include "Logging.h"
#include <iostream>

/**
 * LoggingService implementation
 * 
 * SPECIAL: Initializes logging in constructor before any output
 * This ensures logging-first invariant is maintained
 */

LoggingService::LoggingService() {
    // CRITICAL: Call initLogging() immediately in constructor
    // This is the ONLY place where initLogging() should be called
    // Other services may output only after LoggingService is constructed
    Logging::Initialize();
    
    // Now logging is active - we can output
    // Note: initLogging() itself may output "Logging initialized"
}

LoggingService::~LoggingService() {
    // Stop() should have been called, but ensure cleanup here too
    Stop();
}

void LoggingService::Configure() {
    // No configuration needed - logging is initialized in constructor
}

bool LoggingService::Start() {
    // Logging is already active (initialized in constructor)
    // This is a no-op but follows IService contract
    std::cout << "[DEBUG] LoggingService::Start() - logging already active" << std::endl;
    return true;
}

void LoggingService::Stop() {
    // Cleanup logging resources
    Logging::Cleanup();
}
