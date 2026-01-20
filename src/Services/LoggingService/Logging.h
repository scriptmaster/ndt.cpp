#ifndef LOGGING_H
#define LOGGING_H

#include <string>

/**
 * Logging utilities
 * 
 * Single responsibility: File-based logging infrastructure
 * Handles log file creation, stdout/stderr redirection, and timestamp generation
 */
namespace Logging {
    /**
     * Initialize logging system
     * Creates log file and redirects stdout/stderr to it
     * Hides console window on Windows
     */
    void Initialize();

    /**
     * Cleanup logging system
     * Restores original stdout/stderr buffers and closes log file
     */
    void Cleanup();

    /**
     * Get timestamp string for log filenames
     * Format: YYYY-MM-DD-HH-MM-SS
     * 
     * @return Timestamp string
     */
    std::string GetTimestamp();
}

#endif // LOGGING_H
