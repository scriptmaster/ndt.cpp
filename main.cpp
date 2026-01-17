#include "display/logging.h"
#include "display/window.h"
#include "display/scene.h"
#include "display/audio.h"
#include "display/admin.h"
#include "display/app.h"
#include "display/render.h"
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <iostream>
#include <vector>

/**
 * Main application entry point
 * Initializes all systems, runs main loop, and cleans up on exit
 * All initialization and cleanup are wrapped in error handling
 */

int main() {
    /**
     * Initialize logging system first
     * This hides the console window on Windows and redirects output to log file
     * All subsequent std::cout/cerr will go to logs/run.YYYY-MM-DD-HH-MM-SS.log
     * Must be done before any output to ensure all messages are logged
     */
    std::cout << "[DEBUG] Starting main()..." << std::endl;
    
    /**
     * STEP 1: Initialize logging
     */
    std::cout << "[DEBUG] STEP 1: Initializing logging..." << std::endl;
    try {
        initLogging();
        std::cout << "[DEBUG] STEP 1: Logging initialized - SUCCESS" << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] STEP 1: Logging initialization failed" << std::endl;
        return -1;
    }
    
    /**
     * STEP 2: Check admin status
     */
    std::cout << "[DEBUG] STEP 2: Checking admin status..." << std::endl;
    bool isAdmin = false;
    try {
        isAdmin = isRunningAsAdmin();
        if (isAdmin) {
            std::cout << "[DEBUG] STEP 2: Running as administrator - SUCCESS" << std::endl;
        } else {
            std::cout << "[DEBUG] STEP 2: Not running as admin - SUCCESS" << std::endl;
        }
    } catch (...) {
        std::cerr << "[ERROR] STEP 2: Admin check failed" << std::endl;
        cleanupLogging();
        return -1;
    }
    
    /**
     * STEP 3: Create windows
     */
    std::cout << "[DEBUG] STEP 3: Creating windows..." << std::endl;
    std::vector<WindowData> windows;
    try {
        windows = createWindows();
        std::cout << "[DEBUG] STEP 3: Windows created, count: " << windows.size() << " - SUCCESS" << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] STEP 3: Window creation failed with exception" << std::endl;
        cleanupLogging();
        return -1;
    }
    
    /**
     * STEP 4: Set admin status for all windows
     */
    std::cout << "[DEBUG] STEP 4: Setting admin status for windows..." << std::endl;
    try {
        for (auto& wd : windows) {
            wd.isAdmin = isAdmin;
        }
        std::cout << "[DEBUG] STEP 4: Admin status set - SUCCESS" << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] STEP 4: Setting admin status failed" << std::endl;
        cleanupWindows(windows);
        cleanupLogging();
        return -1;
    }
    
    /**
     * STEP 5: Validate windows
     */
    std::cout << "[DEBUG] STEP 5: Validating windows..." << std::endl;
    if (windows.empty()) {
        std::cerr << "[ERROR] STEP 5: No windows created" << std::endl;
        cleanupLogging();
        return -1;
    }
    std::cout << "[DEBUG] STEP 5: Windows validated - SUCCESS" << std::endl;
    
    /**
     * STEP 6: Set VSync
     */
    std::cout << "[DEBUG] STEP 6: Setting VSync..." << std::endl;
    try {
        glfwSwapInterval(1);
        std::cout << "[DEBUG] STEP 6: VSync set - SUCCESS" << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] STEP 6: VSync setting failed" << std::endl;
        cleanupWindows(windows);
        cleanupLogging();
        return -1;
    }
    
    /**
     * STEP 7: Display startup messages
     */
    std::cout << "[DEBUG] STEP 7: Displaying startup messages..." << std::endl;
    std::cout << "NDT Logo Display Running..." << std::endl;
    std::cout << "Press ESC, Alt+F4, or close windows to exit" << std::endl;
    std::cout << "[DEBUG] STEP 7: Startup messages displayed - SUCCESS" << std::endl;
    
    /**
     * STEP 8: Initialize audio system (DISABLED FOR DEBUGGING)
     */
    std::cout << "[DEBUG] STEP 8: Audio initialization DISABLED for debugging" << std::endl;
    bool audioInitialized = false;
    // DISABLED: Audio initialization temporarily disabled to isolate crash
    /*
    try {
        audioInitialized = initializeAudioSystem();
        if (!audioInitialized) {
            std::cerr << "[WARNING] STEP 8: Audio initialization failed, continuing without audio" << std::endl;
        } else {
            std::cout << "[DEBUG] STEP 8: Audio initialized - SUCCESS" << std::endl;
        }
    } catch (...) {
        std::cerr << "[ERROR] STEP 8: Audio initialization failed with exception" << std::endl;
        // Continue anyway
    }
    */
    
    /**
     * STEP 9: Run main loop
     */
    std::cout << "[DEBUG] STEP 9: Starting main loop..." << std::endl;
    try {
        runMainLoop(windows);
        std::cout << "[DEBUG] STEP 9: Main loop exited - SUCCESS" << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] STEP 9: Main loop failed with exception" << std::endl;
        cleanupWindows(windows);
        cleanupLogging();
        return -1;
    }
    
    /**
     * STEP 10: Cleanup
     */
    std::cout << "[DEBUG] STEP 10: Cleaning up..." << std::endl;
    try {
        cleanupApplication(windows);
        std::cout << "[DEBUG] STEP 10: Cleanup complete - SUCCESS" << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] STEP 10: Cleanup failed with exception" << std::endl;
        return -1;
    }
    
    /**
     * Return success exit code
     * Exit code 0 indicates normal shutdown
     * Non-zero codes indicate errors (handled in cleanup functions)
     */
    return 0;
}
