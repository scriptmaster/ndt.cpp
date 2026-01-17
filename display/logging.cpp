#include "logging.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <cstdlib>

// Global log file stream
static std::ofstream logFile;
static std::streambuf* coutBuf = nullptr;
static std::streambuf* cerrBuf = nullptr;

// Helper function to get timestamp string
std::string getTimestamp() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S");
    return oss.str();
}

// Initialize logging
void initLogging() {
    // Create logs directory if it doesn't exist
    #ifdef _WIN32
    system("if not exist logs mkdir logs");
    #else
    system("mkdir -p logs 2>/dev/null");
    #endif
    
    // Create log filename with timestamp
    std::string logFilename = "logs/run." + getTimestamp() + ".log";
    logFile.open(logFilename, std::ios::app);
    
    if (logFile.is_open()) {
        // Save original buffers
        coutBuf = std::cout.rdbuf();
        cerrBuf = std::cerr.rdbuf();
        
        // Redirect stdout and stderr to log file
        std::cout.rdbuf(logFile.rdbuf());
        std::cerr.rdbuf(logFile.rdbuf());
    }
    
    std::cerr << "Logging initialized" << std::endl;

    // Hide console window on Windows
    #ifdef _WIN32
    FreeConsole();
    std::cerr << "Console hidden" << std::endl;
    #endif
}

// Cleanup logging
void cleanupLogging() {
    // Restore original buffers and close log file
    if (logFile.is_open()) {
        if (coutBuf) std::cout.rdbuf(coutBuf);
        if (cerrBuf) std::cerr.rdbuf(cerrBuf);
        logFile.close();
    }
}
