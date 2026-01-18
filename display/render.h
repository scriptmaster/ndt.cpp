#ifndef RENDER_H
#define RENDER_H

#include "window.h"
#include "scene.h"
#include <vector>

/**
 * Rendering and state management functions
 * Handles all rendering operations and display state transitions
 */

// Forward declarations
struct WindowData;

/**
 * Render a single window for the current frame
 * Handles context switching, framebuffer setup, and delegates to state handlers
 * @param wd Window data for the window to render
 * @param frameCount Current frame number for debug logging
 */
void renderWindow(WindowData& wd, int frameCount);

/**
 * Prepare window for rendering
 * Sets up OpenGL context, gets framebuffer size, and clears the screen
 * @param wd Window data for the window to prepare
 * @param fbWidth Output: framebuffer width in pixels
 * @param fbHeight Output: framebuffer height in pixels
 */
void prepareWindowForRendering(const WindowData& wd, int& fbWidth, int& fbHeight);

/**
 * Handle display state transitions and rendering
 * Manages logo fade-in, showing, fade-out, and scene transitions
 * Updates alpha value based on current state and elapsed time
 * @param wd Window data containing state information
 * @param currentTime Current time from GLFW for timing calculations
 * @param alpha Output: alpha value for logo rendering (0.0 to 1.0)
 */
void handleDisplayState(WindowData& wd, double currentTime, float& alpha);

/**
 * Render content based on current display state
 * Renders either logo texture, opening scene, admin scene, or error placeholder
 * Opening scene is loaded lazily from WindowData when needed
 * @param wd Window data for the window being rendered (contains scene data)
 * @param fbWidth Framebuffer width in pixels
 * @param fbHeight Framebuffer height in pixels
 * @param alpha Alpha value for logo rendering
 * @param lastFrameTime Reference to last frame time for delta calculation
 */
void renderContentForState(WindowData& wd, int fbWidth, int fbHeight, float alpha, double& lastFrameTime, int frameCount);

/**
 * Handle logo fade-in state
 * Gradually increases alpha from 0 to 1 over FADE_IN_DURATION seconds
 * Transitions to LOGO_SHOWING state when fade-in completes
 * @param wd Window data to update
 * @param elapsed Time elapsed since fade started
 * @param currentTime Current time for state transition
 * @return Alpha value for logo rendering (0.0 to 1.0)
 */
float handleLogoFadeIn(WindowData& wd, double elapsed, double currentTime);

/**
 * Handle logo showing state
 * Logo is fully visible (alpha = 1.0)
 * Monitors for clicks and automatically transitions after MAX_SHOW_DURATION
 * Detects double-clicks to change audio seed
 * @param wd Window data to update
 * @param currentTime Current time for click detection and auto-transition
 * @return Alpha value (always 1.0 in this state)
 */
float handleLogoShowing(WindowData& wd, double currentTime);

/**
 * Handle logo fade-out state
 * Gradually decreases alpha from 1 to 0 over FADE_OUT_DURATION seconds
 * Transitions to OPENING_SCENE state when fade-out completes
 * @param wd Window data to update
 * @param elapsed Time elapsed since fade-out started
 * @param currentTime Current time for state transition
 * @return Alpha value for logo rendering (1.0 to 0.0)
 */
float handleLogoFadeOut(WindowData& wd, double elapsed, double currentTime);

/**
 * Handle opening scene state with lazy loading
 * Loads scene on first access and shows loading indicator during load
 * Renders the opening scene with language selection cards once loaded
 * @param wd Window data containing scene and loading state
 * @param fbWidth Framebuffer width in pixels
 * @param fbHeight Framebuffer height in pixels
 * @param lastFrameTime Reference to last frame time for delta calculation
 */
void handleOpeningScene(WindowData& wd, int fbWidth, int fbHeight, double& lastFrameTime, int frameCount);

/**
 * Load opening scene lazily when entering OPENING_SCENE state
 * Shows progress indicator during file system operations
 * Updates loading progress and status throughout the loading process
 * @param wd Window data to store loaded scene and loading state
 */
void loadOpeningSceneLazy(WindowData& wd);

/**
 * Render loading indicator with progress bar and status text
 * Shows animated loading spinner, progress bar, and status messages
 * Used during scene file loading to provide user feedback
 * @param fbWidth Framebuffer width in pixels
 * @param fbHeight Framebuffer height in pixels
 * @param progress Loading progress (0.0 to 1.0)
 * @param status Loading status message
 */
void renderLoadingIndicator(int fbWidth, int fbHeight, float progress, const std::string& status);

/**
 * Render error placeholder when texture fails to load
 * Draws a red rectangle in center of screen to indicate error
 * Used when logo image file is missing or corrupted
 * @param fbWidth Framebuffer width in pixels
 * @param fbHeight Framebuffer height in pixels
 */
void renderErrorPlaceholder(int fbWidth, int fbHeight);

#endif // RENDER_H