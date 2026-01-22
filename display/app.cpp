#include "app.h"
#include "window.h"
#include "scene.h"
#include "audio.h"
#include "network.h"
#include "logging.h"
#include "render.h"
#include "texture.h"
#include "admin.h"
#include "scene_logger.h"
#include "opening_scene.h"
#include "safety/safe_scope.h"
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#include <GLFW/glfw3native.h>
#endif

#include <iostream>
#include <stdexcept>
#include <exception>
#include <cmath>
#include <map>
#include <cstdlib>

/**
 * Initialize all application systems (logging, network, audio generation, audio capture)
 * This function coordinates initialization of multiple subsystems:
 * - Scene logger (for debug logging)
 * - Network (for Whisper STT)
 * - Audio generation (procedural audio based on seed)
 * - Audio capture (microphone input for waveform and STT)
 * 
 * Attempts to load audio seed from config file, falls back to default
 * Wraps initialization in try-catch to handle any exceptions gracefully
 */
bool initializeSystems() {
    // Mark as SafeBoundary - initialization function where exceptions are allowed
    safety::SafeBoundary boundary;
    
    std::cout << "[DEBUG] Initializing audio..." << std::endl;
    
    // Initialize scene logger (which also initializes audio logger)
    initSceneLogger();
    
    /**
     * Attempt to load audio seed from config file
     * If file doesn't exist or load fails, use default seed (12345)
     * This ensures audio always has a valid seed even if config is missing
     */
    try {
        int seed = 12345; // Default seed
        if (!loadAudioSeed("config/audio_seed.txt")) {
            // File doesn't exist or failed to load, use default
            std::cout << "[DEBUG] Using default audio seed: " << seed << std::endl;
        } else {
            seed = getAudioSeed();
            std::cout << "[DEBUG] Loaded audio seed from config: " << seed << std::endl;
        }
        
        /**
         * Initialize network subsystem for Whisper STT
         * This sets up WinSock2 on Windows or prepares network for Unix
         * Must be done before any network operations
         */
        if (!initNetwork()) {
            std::cerr << "[WARNING] Network initialization failed - STT will not work" << std::endl;
        } else {
            std::cout << "[DEBUG] Network initialized successfully" << std::endl;
        }
        
        /**
         * Initialize audio generation system with the determined seed
         * This sets up procedural audio generation based on the seed value
         * Any exceptions during initialization will be caught and logged
         */
        initAudioGeneration(seed);
        std::cout << "[DEBUG] Audio generation initialized successfully" << std::endl;
        
        /**
         * Initialize audio capture for Whisper STT
         * This sets up Windows waveIn API to capture microphone input
         * Audio will be captured at 44.1kHz sample rate
         */
        if (!initAudioCapture(44100)) {
            std::cerr << "[WARNING] Audio capture initialization failed - STT will not receive audio" << std::endl;
        } else {
            std::cout << "[DEBUG] Audio capture initialized successfully" << std::endl;
            // Start capturing audio immediately for STT
            startAudioCapture();
            std::cout << "[DEBUG] Audio capture started" << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during audio initialization: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during audio initialization" << std::endl;
        return false;
    }
}


/**
 * Check if user requested application shutdown
 * Iterates through all windows checking for close requests
 * Returns true if any window should close (user pressed ESC, Alt+F4, or close button)
 */
bool shouldShutdownApplication(const std::vector<WindowData>& windows) {
    /**
     * Check each window for close request
     * GLFW sets window should-close flag when user clicks X, presses ESC (if handled),
     * or when we explicitly call glfwSetWindowShouldClose
     */
    for (const auto& wd : windows) {
        if (glfwWindowShouldClose(wd.window)) {
            std::cout << "[DEBUG] Window close requested, shutting down gracefully..." << std::endl;
            return true;
        }
    }
    return false;
}

/**
 * Process all user input for a single frame
 * Checks for keyboard shortcuts like ESC and Alt+F4 across all windows
 * Sets window close flag when shutdown shortcuts are detected
 */
void processUserInput(const std::vector<WindowData>& windows) {
    /**
     * Check each window for keyboard input
     * We iterate through all windows to catch input on any active window
     * ESC key and Alt+F4 are universal shutdown shortcuts
     */
    for (const auto& wd : windows) {
        try {
            // /**
            //  * Check for ESC key press
            //  * ESC is a common shutdown shortcut, so we handle it here
            //  * When detected, we set all windows to close, ensuring complete shutdown
            //  */
            // if (glfwGetKey(wd.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            //     std::cout << "ESC key pressed - shutting down gracefully..." << std::endl;
            //     for (auto& w : windows) {
            //         glfwSetWindowShouldClose(w.window, GLFW_TRUE);
            //     }
            //     return; // Exit early since we're shutting down
            // }

            /**
             * Check for Alt+F4 key combination
             * Alt+F4 is the standard Windows close shortcut
             * GLFW also handles this via window close callback, but we log it explicitly
             */
            if (glfwGetKey(wd.window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
                glfwGetKey(wd.window, GLFW_KEY_F4) == GLFW_PRESS) {
                std::cout << "Alt+F4 pressed - shutting down gracefully..." << std::endl;
                for (auto& w : windows) {
                    glfwSetWindowShouldClose(w.window, GLFW_TRUE);
                }
                return; // Exit early since we're shutting down
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception checking keys: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] Unknown exception checking keys" << std::endl;
        }
    }
}

/**
 * Update window visibility and focus state
 * Ensures windows remain visible if they get minimized or hidden
 * Primary window can receive focus; secondary windows stay topmost but never focus
 * This prevents windows from disappearing when user uses Alt+Tab or other window managers
 */
void maintainWindowVisibility(std::vector<WindowData>& windows) {
    /**
     * Handle primary window visibility
     * Primary window is the main interactive window (usually the horizontal monitor)
     * It can receive focus and appears in taskbar/Alt+Tab menu
     */
    for (auto& wd : windows) {
        if (wd.isPrimary) {
            /**
             * Check current visibility and iconified state
             * If window was minimized or hidden (e.g., by Alt+Tab), restore it
             * Focus is handled separately by window callbacks to avoid fighting
             */
            int visible = glfwGetWindowAttrib(wd.window, GLFW_VISIBLE);
            int iconified = glfwGetWindowAttrib(wd.window, GLFW_ICONIFIED);
            
            /**
             * Restore window if it's hidden or minimized
             * On Windows, we also check native window visibility for reliability
             * Only restore if actually needed to avoid unnecessary API calls
             */
            if (!visible || iconified) {
                #ifdef _WIN32
                HWND hwnd = glfwGetWin32Window(wd.window);
                if (hwnd && (!IsWindowVisible(hwnd) || iconified)) {
                    glfwRestoreWindow(wd.window);
                    glfwShowWindow(wd.window);
                    // Focus will be handled by callback if needed
                }
                #else
                if (iconified) {
                    glfwRestoreWindow(wd.window);
                }
                if (!visible) {
                    glfwShowWindow(wd.window);
                }
                #endif
            }
        } else {
            /**
             * Handle secondary window visibility
             * Secondary windows (usually vertical monitor) stay topmost but never focus
             * They don't appear in taskbar or Alt+Tab, making them overlay-style
             * Still need to restore if hidden/minimized, but never steal focus
             */
            int visible = glfwGetWindowAttrib(wd.window, GLFW_VISIBLE);
            int iconified = glfwGetWindowAttrib(wd.window, GLFW_ICONIFIED);
            
            /**
             * Restore secondary windows without activating them
             * Use SW_SHOWNOACTIVATE on Windows to show without focus
             * Maintain topmost position without interrupting user's current focus
             */
            if (!visible || iconified) {
                #ifdef _WIN32
                HWND hwnd = glfwGetWin32Window(wd.window);
                if (hwnd && (!IsWindowVisible(hwnd) || iconified)) {
                    glfwRestoreWindow(wd.window);
                    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
                    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
                               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                }
                #else
                if (iconified) {
                    glfwRestoreWindow(wd.window);
                }
                if (!visible) {
                    glfwShowWindow(wd.window);
                }
                #endif
            }
        }
    }
}

/**
 * Run the main application loop
 * Continues until shutdown is requested or all windows are closed
 * Each iteration: renders all windows, processes input, updates audio, maintains visibility
 */
void runMainLoop(std::vector<WindowData>& windows) {
    // Mark as SafeScope - main loop is a no-throw zone
    // Exceptions should not escape from this worker loop
    safety::SafeScope scope;
    
    /**
     * Initialize timing for frame rate calculations
     * Last frame time is used to calculate delta time between frames
     * Delta time makes animations frame-rate independent
     */
    double lastFrameTime = glfwGetTime();
    std::cout << "[DEBUG] Initial lastFrameTime: " << lastFrameTime << std::endl;
    std::cout << "[DEBUG] Entering main loop..." << std::endl;
    
    /**
     * Main application loop
     * Continues running while windows exist and no shutdown requested
     * Each iteration represents one frame of rendering
     */
    int frameCount = 0;
    while (!windows.empty()) {
        try {
            /**
             * Increment frame counter for debug logging
             * Log frame number every 60 frames (approximately once per second at 60fps)
             * This helps track application performance and identify issues
             */
            frameCount++;
            if (frameCount % 60 == 0) {
                std::cout << "[DEBUG] Frame " << frameCount << std::endl;
            }
            
            /**
             * Check if user requested shutdown
             * User can request shutdown via ESC, Alt+F4, or window close button
             * If shutdown requested, break out of main loop immediately
             */
            if (shouldShutdownApplication(windows)) {
                break; // Exit main loop
            }
            
            /**
             * Render each window for this frame
             * Each window is rendered independently with its own OpenGL context
             * Rendering includes logo, scenes, and all visual elements
             */
            for (auto& wd : windows) {
                try {
                    /**
                     * Prepare window context and framebuffer
                     * Sets OpenGL context current and clears screen
                     * Gets actual framebuffer size (important for high-DPI displays)
                     */
                    int fbWidth, fbHeight;
                    prepareWindowForRendering(wd, fbWidth, fbHeight);
                    
                    /**
                     * Get current time for state management
                     * Time is used for fade timing and automatic state transitions
                     * Elapsed time calculations determine fade progress
                     */
                    double currentTime = glfwGetTime();
                    std::cout << "[DEBUG] Current time: " << currentTime << std::endl;
                    double elapsed = currentTime - wd.fadeStartTime;
                    float alpha = 1.0f;
                    
                    /**
                     * Handle display state transitions
                     * State machine manages logo fade-in, showing, fade-out, and scene states
                     * Updates alpha value based on current state and elapsed time
                     */
                    handleDisplayState(wd, currentTime, alpha);
                    
                    /**
                     * Render content based on current state
                     * Routes to logo texture, opening scene, admin scene, or error placeholder
                     * Opening scene is rendered here; logo alpha is used for fade effects
                     */
                    renderContentForState(wd, fbWidth, fbHeight, alpha, lastFrameTime, frameCount);
                    
                    /**
                     * Swap front and back buffers to display rendered frame
                     * Double buffering prevents flickering during rendering
                     * VSync ensures frame rate is limited to monitor refresh rate
                     */
                    std::cout << "[DEBUG] Swapping buffers..." << std::endl;
                    glfwSwapBuffers(wd.window);
                    std::cout << "[DEBUG] Buffers swapped" << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] Exception during window rendering: " << e.what() << std::endl;
                    // Continue to next window
                } catch (...) {
                    std::cerr << "[ERROR] Unknown exception during window rendering" << std::endl;
                    // Continue to next window
                }
            }
            
            /**
             * Process all pending events from GLFW
             * This includes window events, keyboard input, mouse input, etc.
             * Must be called regularly to keep window responsive
             */
            std::cout << "[DEBUG] Polling events..." << std::endl;
            try {
                glfwPollEvents();
                std::cout << "[DEBUG] Events polled" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Exception during event polling: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[ERROR] Unknown exception during event polling" << std::endl;
            }
            
            /**
             * Update audio system for this frame (DISABLED FOR DEBUGGING)
             * Audio updates include waveform generation and procedural sound
             * Delta time ensures audio is frame-rate independent
             */
            std::cout << "[DEBUG] Updating audio..." << std::endl;
            try {
                double currentFrameTime = glfwGetTime();
                float deltaTime = (float)(currentFrameTime - lastFrameTime);
                
                /**
                 * Clamp delta time to prevent invalid values
                 * Very large deltas can occur if application was paused
                 * Invalid deltas (NaN, infinity) can break animations and audio
                 */
                if (deltaTime < 0.0f || deltaTime > 1.0f || !std::isfinite(deltaTime)) {
                    deltaTime = 0.016f; // Default to ~60fps frame time
                }
                
                lastFrameTime = currentFrameTime;
                updateAudio(deltaTime); // Re-enabled for waveform widget
                std::cout << "[DEBUG] Audio updated" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Exception during audio update: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[ERROR] Unknown exception during audio update" << std::endl;
            }
            
            /**
             * Maintain window visibility and focus
             * Ensures windows remain visible if minimized or hidden
             * Only primary window receives focus; secondary windows stay topmost
             */
            maintainWindowVisibility(windows);
            
            /**
             * Process user input for this frame
             * Checks for keyboard shortcuts like ESC and Alt+F4
             * Sets shutdown flag if user requests application exit
             */
            processUserInput(windows);
            
        } catch (const std::exception& e) {
            /**
             * Catch any exceptions in main loop iteration
             * Log error but continue loop to keep application running
             * This prevents single-frame errors from crashing entire application
             */
            std::cerr << "[ERROR] Exception in main loop iteration: " << e.what() << std::endl;
            // Continue loop
        } catch (...) {
            std::cerr << "[ERROR] Unknown exception in main loop iteration" << std::endl;
            // Continue loop
        }
    }
}

/**
 * Cleanup all application resources in proper order
 * Shuts down windows first, then audio, then logging, finally GLFW
 * Each cleanup step is wrapped in try-catch to ensure all resources are attempted
 * This ensures graceful shutdown even if some cleanup steps fail
 */
void cleanupApplication(std::vector<WindowData>& windows) {
    std::cout << "NDT Logo Display shutting down gracefully..." << std::endl;
    std::cout << "[DEBUG] Starting cleanup..." << std::endl;
    
    /**
     * Cleanup windows and release OpenGL contexts
     * This closes all windows and destroys their GLFW resources
     * Must happen before GLFW termination to avoid resource leaks
     */
    try {
        cleanupWindows(windows);
        std::cout << "[DEBUG] Windows cleaned up" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during window cleanup: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during window cleanup" << std::endl;
    }
    
    /**
     * Stop and cleanup audio capture
     * Releases Windows waveIn resources and buffers
     * Must be done before audio cleanup
     */
    try {
        stopAudioCapture();
        cleanupAudioCapture();
        std::cout << "[DEBUG] Audio capture cleaned up" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during audio capture cleanup: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during audio capture cleanup" << std::endl;
    }
    
    /**
     * Cleanup audio system resources
     * Releases any audio buffers and closes audio device
     * Safe to call even if audio was never initialized
     */
    try {
        cleanupAudio();
        std::cout << "[DEBUG] Audio cleaned up" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during audio cleanup: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during audio cleanup" << std::endl;
    }
    
    /**
     * Cleanup network subsystem
     * On Windows, cleans up WinSock2
     * Must be done after all network operations are complete
     */
        try {
            cleanupNetwork();
            std::cout << "[DEBUG] Network cleaned up" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception during network cleanup: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] Unknown exception during network cleanup" << std::endl;
        }
        
        /**
         * Cleanup scene logger (which also closes audio logger)
         * This ensures all log files are properly closed
         */
        try {
            cleanupSceneLogger();
            std::cout << "[DEBUG] Scene logger cleaned up" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception during scene logger cleanup: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] Unknown exception during scene logger cleanup" << std::endl;
        }
    
    /**
     * Cleanup scene and audio loggers
     * Closes scene.log and audio.log files
     * Must be done before main logging cleanup
     */
    try {
        cleanupSceneLogger();
        cleanupAudioLogger();
        std::cout << "[DEBUG] Scene and audio loggers cleaned up" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during scene/audio logger cleanup: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during scene/audio logger cleanup" << std::endl;
    }
    
    /**
     * Cleanup logging system
     * Restores stdout/stderr redirection and closes log file
     * This allows final messages to be written before file closes
     */
    try {
        cleanupLogging();
        std::cout << "[DEBUG] Logging cleaned up" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during logging cleanup: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during logging cleanup" << std::endl;
    }
    
    /**
     * Finally terminate GLFW library
     * This must be last as it destroys all GLFW resources globally
     * After this call, no GLFW functions can be used
     */
    try {
        glfwTerminate();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during GLFW termination: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during GLFW termination" << std::endl;
    }
    
    std::cout << "[DEBUG] Exiting main()..." << std::endl;
}
