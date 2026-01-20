#ifndef LOGGINGSERVICE_H
#define LOGGINGSERVICE_H

#include "ILoggingService.h"
#include "Logging.h"

/**
 * LoggingService - Logging service implementation
 * 
 * SPECIAL SERVICE: Calls initLogging() in constructor
 * This is the ONLY service allowed to output before construction
 * Other services may only log AFTER LoggingService exists
 * 
 * Following .NET's ILoggingService pattern
 */
class LoggingService : public ILoggingService {
public:
    /**
     * Constructor - initializes logging immediately
     * MUST be called first before any other service
     * Calls initLogging() which redirects stdout/stderr to log file
     * 
     * CRITICAL: This is the only constructor that may call initLogging()
     * No other service constructor should output anything
     */
    LoggingService();

    /**
     * Destructor - cleans up logging
     */
    ~LoggingService();

    /**
     * Configure logging service
     * No configuration needed (logging initialized in constructor)
     */
    void Configure() override;

    /**
     * Start logging service
     * Logging is already active (initialized in constructor)
     * This is a no-op but follows IService contract
     * 
     * @return true (logging is always started in constructor)
     */
    bool Start() override;

    /**
     * Stop logging service
     * Cleans up logging resources
     */
    void Stop() override;
};

#endif // LOGGINGSERVICE_H
