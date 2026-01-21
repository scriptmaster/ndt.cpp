#include "AppLoop.h"
#include "WindowData.h"
#include "WindowManager.h"
#include "Renderer.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <exception>
#include <cmath>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#include <GLFW/glfw3native.h>
#endif

#include "../AudioCaptureService/AudioWaveform.h"
#include "../AudioCaptureService/AudioCapture.h"
#include "../AudioProcessorService/AudioProcessorService.h"
#include "../STTService/STTService.h"

/**
 * AppLoop - Main application loop and input handling
 * 
 * Single responsibility: Run main application loop, process input, maintain window visibility
 * Ported from display/app.cpp
 */

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
        if (glfwWindowShouldClose(wd.window)) {
            continue;
        }
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
                std::cout << "[DEBUG] Audio callbacks: " << getAudioCaptureCallbackCount()
                          << " zero-bytes: " << getAudioCaptureZeroByteCount()
                          << " silent: " << ((AudioProcessorService::GetInstance() &&
                                             AudioProcessorService::GetInstance()->IsSilent()) ? "yes" : "no") << std::endl;
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
             * Process all pending events from GLFW
             * This includes window events, keyboard input, mouse input, etc.
             * Must be called regularly to keep window responsive
             */
            try {
                glfwPollEvents();
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Exception during event polling: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[ERROR] Unknown exception during event polling" << std::endl;
            }

            /**
             * Process user input for this frame
             * Checks for keyboard shortcuts like ESC and Alt+F4
             * Sets shutdown flag if user requests application exit
             */
            processUserInput(windows);

            /**
             * If shutdown is requested after input/events, exit immediately
             * Avoids re-showing windows or continuing render loop
             */
            if (shouldShutdownApplication(windows)) {
                break;
            }

            // STT is executed inside AudioProcessorService on speech end.

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
                    double elapsed = currentTime - wd.fadeStartTime;
                    (void)elapsed;
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
                    glfwSwapBuffers(wd.window);
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] Exception during window rendering: " << e.what() << std::endl;
                    // Continue to next window
                } catch (...) {
                    std::cerr << "[ERROR] Unknown exception during window rendering" << std::endl;
                    // Continue to next window
                }
            }
            
            /**
             * Update audio system for this frame
             * Audio updates include waveform generation and procedural sound
             * Delta time ensures audio is frame-rate independent
             */
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
        } // end try
    } // end while
    std::cout << "[DEBUG] Main loop exited - SUCCESS" << std::endl;
}
