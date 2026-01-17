#ifndef APP_H
#define APP_H

#include "window.h"
#include "scene.h"
#include <vector>
#include <string>

/**
 * Application lifecycle and main loop management
 * Provides high-level functions to initialize, run, and cleanup the application
 */

// Forward declarations
struct WindowData;

/**
 * Initialize the audio system with error handling
 * Attempts to load audio seed from config file, falls back to default
 * @return true if initialization succeeded, false otherwise
 */
bool initializeAudioSystem();

/**
 * Load the opening scene from JSON file (DEPRECATED - use lazy loading instead)
 * This function is no longer used - scenes are loaded lazily when needed
 * Kept for backward compatibility but should not be called
 */
bool loadOpeningScene(Scene& openingScene);

/**
 * Check if user requested application shutdown
 * Checks all windows for close requests (ESC, Alt+F4, window close button)
 * @param windows Vector of all application windows
 * @return true if shutdown was requested, false otherwise
 */
bool shouldShutdownApplication(const std::vector<WindowData>& windows);

/**
 * Process all user input for a single frame
 * Checks for keyboard shortcuts (ESC, Alt+F4) and handles them
 * @param windows Vector of all application windows
 */
void processUserInput(const std::vector<WindowData>& windows);

/**
 * Update window visibility and focus state
 * Ensures windows remain visible and properly focused
 * Only primary window receives focus; secondary windows stay topmost but unfocused
 * @param windows Vector of all application windows
 */
void maintainWindowVisibility(std::vector<WindowData>& windows);

/**
 * Run the main application loop
 * Handles rendering, state transitions, input, and audio updates
 * Continues until shutdown is requested or windows are closed
 * Opening scenes are loaded lazily when state transitions to OPENING_SCENE
 * @param windows Vector of all application windows (contains scene data)
 */
void runMainLoop(std::vector<WindowData>& windows);

/**
 * Cleanup all application resources
 * Properly shuts down windows, audio, logging, and GLFW in reverse order
 * All cleanup operations are wrapped in try-catch for safety
 * @param windows Vector of all application windows
 */
void cleanupApplication(std::vector<WindowData>& windows);

#endif // APP_H