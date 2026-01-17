#include "render.h"
#include "window.h"
#include "scene.h"
#include "texture.h"
#include "audio.h"
#include "admin.h"
#include <cstdio>  // For FILE, fopen, fclose
#include <GLFW/glfw3.h>
#include <fstream>
#include <cstdio>
#include <cmath>

#ifdef _WIN32
#include <GL/gl.h>
#include <GLFW/glfw3native.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <iostream>
#include <map>
#include <stdexcept>
#include <exception>

/**
 * Prepare window for rendering
 * Sets up OpenGL context for this window and gets actual framebuffer dimensions
 * Framebuffer size may differ from window size on high-DPI displays
 */
void prepareWindowForRendering(const WindowData& wd, int& fbWidth, int& fbHeight) {
    /**
     * Make this window's OpenGL context current
     * Each window has its own context, so we must switch before rendering
     * This is required for multi-monitor rendering
     */
    glfwMakeContextCurrent(wd.window);
    std::cout << "[DEBUG] Context made current" << std::endl;
    
    /**
     * Get actual framebuffer size (handles high-DPI displays)
     * On Retina displays, framebuffer size is 2x window size
     * This ensures we render at full resolution, not scaled down
     */
    glfwGetFramebufferSize(wd.window, &fbWidth, &fbHeight);
    std::cout << "[DEBUG] Framebuffer size: " << fbWidth << "x" << fbHeight << std::endl;
    
    /**
     * Set background color based on monitor orientation
     * Horizontal monitors (1080p) use white background for logo
     * Vertical monitors (4K 9:16) use black background for logo
     * This provides appropriate contrast for each monitor type
     */
    if (wd.isVertical) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Black for vertical monitors
    } else {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);  // White for horizontal (1080p) monitors
    }
    
    /**
     * Clear the screen with the background color
     * This ensures a clean slate before rendering each frame
     * Must be done before any other rendering operations
     */
    std::cout << "[DEBUG] Clearing screen..." << std::endl;
    glClear(GL_COLOR_BUFFER_BIT);
    std::cout << "[DEBUG] Screen cleared" << std::endl;
}

/**
 * Handle logo fade-in state
 * Logo gradually appears from transparent to fully opaque
 * Uses linear interpolation over 0.8 seconds for smooth fade
 */
float handleLogoFadeIn(WindowData& wd, double elapsed, double currentTime) {
    std::cout << "[DEBUG] State: LOGO_FADE_IN" << std::endl;
    
    /**
     * Calculate fade progress from 0.0 to 1.0
     * Linear interpolation: elapsed time divided by fade duration
     * Clamped to 1.0 maximum to prevent overshoot
     */
    const double FADE_IN_DURATION = 0.8;
    float alpha = (float)std::min(elapsed / FADE_IN_DURATION, 1.0);
    
    /**
     * Transition to showing state when fade completes
     * When alpha reaches 1.0, logo is fully visible
     * Record transition time for showing state duration tracking
     */
    if (alpha >= 1.0f) {
        wd.state = DisplayState::LOGO_SHOWING;
        wd.stateStartTime = currentTime;
    }
    
    return alpha;
}

/**
 * Handle logo showing state
 * Logo is fully visible and waits for user interaction or timeout
 * Detects mouse clicks and double-clicks for audio seed changes
 * Automatically transitions after 20 seconds if no interaction
 */
float handleLogoShowing(WindowData& wd, double currentTime) {
    /**
     * Logo is fully opaque in showing state
     * Alpha is always 1.0 since logo fade-in completed
     */
    float alpha = 1.0f;
    const double MAX_SHOW_DURATION = 20.0; // Wait up to 20 seconds
    const double MIN_SHOW_DURATION = 0.5;   // Minimum brief show
    
    /**
     * Track mouse button state for click detection
     * We need to detect button press transitions (not held state)
     * Static map persists across frames to track previous state
     */
    static std::map<GLFWwindow*, bool> lastMouseState;
    int mouseButton = glfwGetMouseButton(wd.window, GLFW_MOUSE_BUTTON_LEFT);
    bool currentMouseState = (mouseButton == GLFW_PRESS);
    bool wasPressed = lastMouseState[wd.window];
    
    /**
     * Detect click event (button press transition)
     * A click occurs when button was not pressed, now is pressed
     * Get cursor position for double-click detection and logging
     */
    if (currentMouseState && !wasPressed) {
        double xpos, ypos;
        glfwGetCursorPos(wd.window, &xpos, &ypos);
        double clickTime = currentTime;
        
        /**
         * Check for double-click pattern
         * Double-click: two clicks within 0.5 seconds at similar position
         * Distance threshold is 10 pixels to allow slight mouse movement
         * Double-click changes audio seed for variety in procedural audio
         */
        const double DOUBLE_CLICK_TIME = 0.5;
        const double DOUBLE_CLICK_DISTANCE = 10.0; // pixels
        
        if (clickTime - wd.lastClickTime < DOUBLE_CLICK_TIME &&
            std::abs(xpos - wd.lastClickX) < DOUBLE_CLICK_DISTANCE &&
            std::abs(ypos - wd.lastClickY) < DOUBLE_CLICK_DISTANCE) {
            /**
             * Double-click detected - change audio seed
             * Generate new random seed based on current seed
             * Save to config file so change persists across sessions
             * This allows users to "randomize" the audio by double-clicking logo
             */
            int newSeed = getAudioSeed() + (rand() % 10000);
            setAudioSeed(newSeed);
            saveAudioSeed("config/audio_seed.txt");
            std::cout << "[DEBUG] Double-click detected - Audio seed changed to: " << newSeed << std::endl;
        }
        
        /**
         * Any click (single or double) triggers scene loading
         * Store click information for double-click detection next time
         * Start loading scene immediately on click/touch
         * Scene loading happens while logo is still visible
         */
        wd.clickDetected = true;
        wd.lastClickTime = clickTime;
        wd.lastClickX = xpos;
        wd.lastClickY = ypos;
        std::cout << "[DEBUG] Click detected at (" << xpos << ", " << ypos 
                  << ") - starting scene loading" << std::endl;
        
        /**
         * Start lazy loading scene immediately on click
         * This happens while logo is still visible
         * Loading indicator will be shown over the logo
         */
        if (!wd.sceneLoading && !wd.sceneLoaded) {
            loadOpeningSceneLazy(wd);
        }
    }
    lastMouseState[wd.window] = currentMouseState;
    
    /**
     * If scene is loading, show loading indicator and wait for completion
     * Loading indicator is shown over the logo
     * Transition to opening scene once loading completes
     */
    if (wd.sceneLoading) {
        /**
         * Scene is currently loading
         * Stay in showing state until loading completes
         * Loading indicator will be rendered over the logo in renderContentForState
         */
        return 1.0f; // Keep logo visible while loading
    }
    
    /**
     * If scene finished loading, transition directly to opening scene
     * Skip fade-out - go straight to opening scene once loaded
     * This provides immediate feedback after scene loads
     */
    if (wd.sceneLoaded && wd.clickDetected) {
        wd.state = DisplayState::OPENING_SCENE;
        wd.stateStartTime = currentTime;
        std::cout << "[DEBUG] Scene loaded - transitioning to OPENING_SCENE" << std::endl;
        return 1.0f;
    }
    
    /**
     * Auto-transition after 20 seconds if no interaction
     * This provides a fallback if user doesn't click
     * Still loads scene on timeout but user can click earlier
     */
    double showElapsed = currentTime - wd.stateStartTime;
    if (showElapsed >= MAX_SHOW_DURATION) {
        /**
         * 20 seconds elapsed - start loading scene automatically
         * This ensures scene loads even without user interaction
         */
        if (!wd.sceneLoading && !wd.sceneLoaded) {
            loadOpeningSceneLazy(wd);
        }
    }
    
    return alpha;
}

/**
 * Handle logo fade-out state
 * Logo gradually disappears from fully opaque to transparent
 * Uses linear interpolation over 2.0 seconds for smooth fade
 */
float handleLogoFadeOut(WindowData& wd, double elapsed, double currentTime) {
    /**
     * Calculate fade progress from 1.0 to 0.0
     * Linear interpolation: 1.0 minus (elapsed time divided by fade duration)
     * Clamped to 0.0 minimum to prevent negative alpha
     */
    const double FADE_OUT_DURATION = 2.0;
    double fadeOutElapsed = currentTime - wd.stateStartTime;
    float alpha = (float)std::max(1.0 - (fadeOutElapsed / FADE_OUT_DURATION), 0.0);
    
    /**
     * Transition to opening scene when fade completes
     * When alpha reaches 0.0, logo is fully transparent
     * Record transition time for scene rendering
     */
    if (alpha <= 0.0f) {
        wd.state = DisplayState::OPENING_SCENE;
        wd.stateStartTime = currentTime;
    }
    
    return alpha;
}

/**
 * Render loading indicator with progress bar and status text
 * Shows animated loading spinner, progress bar, and status messages
 * Used during scene file loading to provide user feedback
 */
void renderLoadingIndicator(int fbWidth, int fbHeight, float progress, const std::string& status) {
    /**
     * Set up viewport and projection for 2D rendering
     * Use orthographic projection with origin at bottom-left
     * This matches the scene rendering coordinate system
     */
    glViewport(0, 0, fbWidth, fbHeight);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, fbWidth, 0.0, fbHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    /**
     * Enable alpha blending for smooth transparency
     * Needed for semi-transparent progress bars and text
     * Standard alpha blending formula
     */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    /**
     * Render dark background overlay
     * Semi-transparent black background for loading screen
     * Makes loading indicator stand out from any background
     */
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f((float)fbWidth, 0.0f);
        glVertex2f((float)fbWidth, (float)fbHeight);
        glVertex2f(0.0f, (float)fbHeight);
    glEnd();
    
    /**
     * Calculate center position for loading indicator
     * Center vertically and horizontally on screen
     * Progress bar width is 50% of screen width
     */
    float centerX = fbWidth * 0.5f;
    float centerY = fbHeight * 0.5f;
    float barWidth = fbWidth * 0.5f;
    float barHeight = 20.0f;
    float barX = centerX - barWidth * 0.5f;
    float barY = centerY - barHeight * 0.5f;
    
    /**
     * Render progress bar background (dark gray)
     * Background shows full bar extent
     * Gives visual indication of progress range
     */
    glColor4f(0.3f, 0.3f, 0.3f, 0.8f);
    glBegin(GL_QUADS);
        glVertex2f(barX, barY);
        glVertex2f(barX + barWidth, barY);
        glVertex2f(barX + barWidth, barY + barHeight);
        glVertex2f(barX, barY + barHeight);
    glEnd();
    
    /**
     * Render progress bar fill (cyan/blue)
     * Fill width is based on progress (0.0 to 1.0)
     * Animated color transitions as progress increases
     */
    float fillWidth = barWidth * std::max(0.0f, std::min(1.0f, progress));
    float r = 0.2f + progress * 0.6f;  // Dark to bright
    float g = 0.8f;
    float b = 1.0f;
    glColor4f(r, g, b, 0.9f);
    glBegin(GL_QUADS);
        glVertex2f(barX, barY);
        glVertex2f(barX + fillWidth, barY);
        glVertex2f(barX + fillWidth, barY + barHeight);
        glVertex2f(barX, barY + barHeight);
    glEnd();
    
    /**
     * Render animated loading spinner (rotating circle)
     * Position above progress bar
     * Rotates continuously to indicate loading activity
     */
    float spinnerRadius = 30.0f;
    float spinnerY = barY + barHeight + 40.0f;
    static float spinnerRotation = 0.0f;
    spinnerRotation += 0.05f; // Rotate each frame
    if (spinnerRotation > 3.14159f * 2.0f) spinnerRotation -= 3.14159f * 2.0f;
    
    /**
     * Draw spinner as a circle with rotating highlight
     * Circle outline shows loading indicator
     * Highlight rotates to show activity
     */
    glColor4f(0.2f, 0.8f, 1.0f, 0.8f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 32; i++) {
        float angle = (float)i / 32.0f * 3.14159f * 2.0f;
        float highlight = (sinf(angle + spinnerRotation) + 1.0f) * 0.5f;
        float alpha = 0.3f + highlight * 0.5f;
        glColor4f(0.2f, 0.8f, 1.0f, alpha);
        glVertex2f(centerX + cosf(angle) * spinnerRadius, 
                   spinnerY + sinf(angle) * spinnerRadius);
    }
    glEnd();
    
    /**
     * Restore matrices and disable blending
     * Clean up OpenGL state after rendering
     * Prevents affecting subsequent rendering operations
     */
    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    // Note: Status text rendering would require a font/text rendering system
    // For now, we just show the progress bar and spinner
}

/**
 * Load opening scene lazily when entering OPENING_SCENE state
 * Shows progress indicator during file system operations
 * Updates loading progress and status throughout the loading process
 */
void loadOpeningSceneLazy(WindowData& wd) {
    /**
     * Check if scene is already loaded or currently loading
     * Avoid multiple simultaneous load attempts
     * If already loaded, return immediately
     */
    if (wd.sceneLoaded) {
        return; // Already loaded
    }
    if (wd.sceneLoading) {
        return; // Already loading in progress
    }
    
    /**
     * Mark scene as loading to prevent duplicate loads
     * Allocate scene structure on heap (will be freed in cleanup)
     * Initialize loading progress and status
     */
    wd.sceneLoading = true;
    wd.loadingProgress = 0.0f;
    wd.loadingStatus = "Initializing...";
    
    /**
     * Allocate scene structure on heap
     * Scene is stored as pointer to allow lazy allocation
     * Memory will be freed when window is cleaned up
     */
    if (!wd.openingScene) {
        wd.openingScene = new Scene();
    }
    
    /**
     * Update progress: Checking file exists
     * First step is to verify scene file exists
     * Progress: 10% complete
     */
    wd.loadingProgress = 0.1f;
    wd.loadingStatus = "Checking file...";
    std::cout << "[DEBUG] Lazy loading scene: Checking file existence" << std::endl;
    
    /**
     * Check if scene file exists
     * Use C file I/O to avoid potential C++ stream issues
     * This prevents crashes that occurred with std::ifstream
     */
    std::string filename = "scenes/opening.scene.json";
    FILE* file = nullptr;
    
    /**
     * Try to open file to check existence
     * Use "r" mode for reading text file
     * If file doesn't exist, set error status and return
     */
    wd.loadingProgress = 0.2f;
    wd.loadingStatus = "Opening file...";
    std::cout << "[DEBUG] Lazy loading scene: Opening file " << filename << std::endl;
    
    file = fopen(filename.c_str(), "r");
    if (!file) {
        /**
         * File doesn't exist or can't be opened
         * Mark loading as failed and update status
         * Application will continue without scene
         */
        wd.loadingStatus = "Error: File not found";
        wd.sceneLoading = false;
        wd.sceneLoaded = false;
        std::cerr << "[ERROR] Lazy loading scene: Failed to open file " << filename << std::endl;
        return;
    }
    
    /**
     * File opened successfully - progress: 30%
     * Close file handle (we'll use loadScene function which handles its own file I/O)
     * The loadScene function will reopen the file for parsing
     */
    fclose(file);
    file = nullptr;
    
    /**
     * Update progress: Loading scene data
     * Progress: 50% complete
     * Call loadScene function to parse JSON file
     */
    wd.loadingProgress = 0.5f;
    wd.loadingStatus = "Loading scene data...";
    std::cout << "[DEBUG] Lazy loading scene: Parsing JSON file" << std::endl;
    
    /**
     * Call loadScene to parse JSON and populate scene structure
     * loadScene handles file I/O and JSON parsing
     * Returns true if scene loaded successfully
     */
    bool loaded = loadScene(filename, *wd.openingScene);
    
    /**
     * Update progress based on load result
     * If successful, progress is 100% and status shows success
     * If failed, status shows error message
     */
    wd.sceneLoading = false;
    if (loaded) {
        /**
         * Scene loaded successfully
         * Mark as loaded and update status
         * Progress: 100% complete
         */
        wd.loadingProgress = 1.0f;
        wd.loadingStatus = "Scene loaded successfully";
        wd.sceneLoaded = true;
        std::cout << "[DEBUG] Lazy loading scene: Successfully loaded scene" << std::endl;
    } else {
        /**
         * Scene loading failed
         * Mark as not loaded and show error status
         * Progress remains at last successful step
         */
        wd.loadingStatus = "Error: Failed to parse scene file";
        wd.sceneLoaded = false;
        std::cerr << "[ERROR] Lazy loading scene: Failed to parse scene file" << std::endl;
    }
}

/**
 * Handle opening scene state with lazy loading
 * Loads scene on first access and shows loading indicator during load
 * Renders the opening scene with language selection cards once loaded
 */
void handleOpeningScene(WindowData& wd, int fbWidth, int fbHeight, double& lastFrameTime) {
    /**
     * Check if scene needs to be loaded
     * On first entry to OPENING_SCENE state, trigger lazy loading
     * Loading happens incrementally across multiple frames
     */
    if (!wd.sceneLoaded && !wd.sceneLoading) {
        /**
         * Start lazy loading process
         * Loading will happen incrementally across frames
         * Progress indicator will be shown during loading
         */
        loadOpeningSceneLazy(wd);
    }
    
    /**
     * If scene is currently loading, show loading indicator
     * Display progress bar and status message
     * Continue loading process incrementally
     */
    if (wd.sceneLoading) {
        /**
         * Update loading progress if needed
         * Progress updates happen during loadOpeningSceneLazy
         * Render loading indicator with current progress
         */
        renderLoadingIndicator(fbWidth, fbHeight, wd.loadingProgress, wd.loadingStatus);
        return; // Don't render scene until loading completes
    }
    
    /**
     * If scene failed to load, show error message
     * Display error indicator instead of scene
     * Allow application to continue running
     */
    if (!wd.sceneLoaded || !wd.openingScene) {
        /**
         * Render error placeholder instead of scene
         * Shows that scene could not be loaded
         * Application continues without scene functionality
         */
        renderLoadingIndicator(fbWidth, fbHeight, 0.0f, "Error: Scene failed to load");
        return;
    }
    
    /**
     * Scene is loaded - render it normally
     * Calculate delta time for animation and procedural graphics
     * Delta time is time since last frame in seconds
     */
    std::cout << "[DEBUG] State: OPENING_SCENE (rendering loaded scene)" << std::endl;
    try {
        double currentFrameTime = glfwGetTime();
        float deltaTime = (float)(currentFrameTime - lastFrameTime);
        lastFrameTime = currentFrameTime;
        
        /**
         * Clamp delta time to prevent invalid values
         * Very large deltas can occur if application was paused
         * Invalid deltas (NaN, infinity) can break animations
         */
        if (deltaTime < 0.0f || deltaTime > 1.0f || !std::isfinite(deltaTime)) {
            deltaTime = 0.016f; // Default to ~60fps frame time
        }
        
        std::cout << "[DEBUG] DeltaTime: " << deltaTime << std::endl;
        
        /**
         * Render the loaded scene
         * Scene rendering includes background graphics, widgets, and waveform
         * All rendering operations are wrapped in try-catch for safety
         */
        std::cout << "[DEBUG] Calling renderScene..." << std::endl;
        renderScene(*wd.openingScene, fbWidth, fbHeight, deltaTime);
        std::cout << "[DEBUG] renderScene completed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during OPENING_SCENE rendering: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during OPENING_SCENE rendering" << std::endl;
    }
}

/**
 * Handle display state transitions and rendering
 * Main state machine that manages logo and scene display states
 * Updates alpha value based on current state and elapsed time
 */
void handleDisplayState(WindowData& wd, double currentTime, float& alpha) {
    /**
     * Calculate elapsed time since state started
     * This is used for fade timing and automatic transitions
     */
    double elapsed = currentTime - wd.fadeStartTime;
    
    std::cout << "[DEBUG] Current state: " << (int)wd.state << std::endl;
    
    /**
     * Route to appropriate state handler based on current state
     * Each state has its own handler function for clarity
     * State transitions are handled within each state handler
     */
    if (wd.state == DisplayState::LOGO_FADE_IN) {
        /**
         * Logo is fading in from transparent to opaque
         * Alpha increases from 0.0 to 1.0 over 0.8 seconds
         * Transitions to LOGO_SHOWING when complete
         */
        alpha = handleLogoFadeIn(wd, elapsed, currentTime);
    } else if (wd.state == DisplayState::LOGO_SHOWING) {
        /**
         * Logo is fully visible and waiting for interaction
         * Monitors for clicks and auto-transitions after 20 seconds
         * Alpha is always 1.0 in this state
         */
        alpha = handleLogoShowing(wd, currentTime);
    } else if (wd.state == DisplayState::LOGO_FADE_OUT) {
        /**
         * Logo is fading out from opaque to transparent
         * Alpha decreases from 1.0 to 0.0 over 2.0 seconds
         * Transitions to OPENING_SCENE when complete
         */
        alpha = handleLogoFadeOut(wd, elapsed, currentTime);
    }
    /**
     * OPENING_SCENE state is handled separately in renderContentForState
     * because it doesn't affect alpha (it renders a different scene entirely)
     */
}

/**
 * Render content based on current display state
 * Routes to appropriate renderer based on window state
 * Handles logo texture, scene rendering, admin mode, and error states
 */
void renderContentForState(WindowData& wd, int fbWidth, int fbHeight, float alpha, double& lastFrameTime) {
    /**
     * Handle opening scene state
     * Opening scene renders language selection cards with procedural background
     * This is rendered instead of logo texture, so we skip logo rendering
     * Scene is loaded lazily on first access
     */
    if (wd.state == DisplayState::OPENING_SCENE) {
        handleOpeningScene(wd, fbWidth, fbHeight, lastFrameTime);
        return; // Skip logo texture rendering for scene state
    }
    
    /**
     * Handle admin scene state
     * Admin scene is rendered separately and doesn't use logo texture
     * Admin mode is accessed via tetra-click in top-right corner
     */
    if (wd.state == DisplayState::ADMIN_SCENE) {
        /**
         * Admin scene rendering
         * Admin mode displays configuration and control interface
         * Currently handles scene loading and rendering
         */
        try {
            double currentFrameTime = glfwGetTime();
            float deltaTime = (float)(currentFrameTime - lastFrameTime);
            lastFrameTime = currentFrameTime;
            
            /**
             * Load and render admin scene based on current admin scene file
             * Admin scene file is set when user enters admin mode via tetra-click
             * Static scene data persists across frames for performance
             */
            static Scene adminScene;
            static bool adminSceneLoaded = false;
            static std::string lastAdminSceneFile;
            
            /**
             * Reload admin scene if scene file changed
             * This happens when user clicks different tabs in admin mode
             * Each tab loads a different admin scene JSON file
             */
            if (!adminSceneLoaded || lastAdminSceneFile != wd.currentAdminScene) {
                try {
                    adminSceneLoaded = loadAdminScene(wd.currentAdminScene, adminScene);
                    lastAdminSceneFile = wd.currentAdminScene;
                    if (!adminSceneLoaded) {
                        std::cerr << "Error: Failed to load admin scene: " << wd.currentAdminScene << std::endl;
                        wd.state = DisplayState::LOGO_SHOWING; // Fallback to logo
                    }
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] Exception loading admin scene: " << e.what() << std::endl;
                    wd.state = DisplayState::LOGO_SHOWING; // Fallback to logo
                } catch (...) {
                    std::cerr << "[ERROR] Unknown exception loading admin scene" << std::endl;
                    wd.state = DisplayState::LOGO_SHOWING; // Fallback to logo
                }
            }
            
            /**
             * Render admin scene if successfully loaded
             * Admin scene includes tabs, controls, and configuration options
             * Scene is rendered using same renderer as opening scene
             */
            if (adminSceneLoaded) {
                renderScene(adminScene, fbWidth, fbHeight, deltaTime);
            }
            
            /**
             * Render "admin mode" indicator if running as admin
             * Shows red text at bottom-left corner to indicate admin mode is active
             * Helps user understand they're in privileged mode
             */
            if (wd.isAdmin) {
                renderAdminModeText(fbWidth, fbHeight);
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception during ADMIN_SCENE rendering: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] Unknown exception during ADMIN_SCENE rendering" << std::endl;
        }
        return; // Skip logo texture rendering for admin state
    }
    
    /**
     * Render logo texture if window has valid texture
     * Logo is rendered at 50% resolution, centered, maintaining aspect ratio
     * Alpha value controls transparency for fade-in/fade-out effects
     */
    if (wd.isValid) {
        std::cout << "[DEBUG] Rendering texture, alpha: " << alpha << std::endl;
        renderTexture(wd.texture, wd.textureWidth, wd.textureHeight, 
                     fbWidth, fbHeight, alpha);
        std::cout << "[DEBUG] Texture rendered" << std::endl;
        
        /**
         * If scene is loading, show loading indicator over the logo
         * This provides visual feedback that scene is loading in response to click
         * Loading indicator appears as overlay while logo remains visible
         */
        if (wd.sceneLoading) {
            renderLoadingIndicator(fbWidth, fbHeight, wd.loadingProgress, wd.loadingStatus);
        }
    } else {
        /**
         * Render error placeholder if texture failed to load
         * Draws a red rectangle in center of screen
         * Indicates that logo image file is missing or corrupted
         */
        renderErrorPlaceholder(fbWidth, fbHeight);
    }
}

/**
 * Render error placeholder when texture fails to load
 * Draws a red rectangle in center quarter of screen
 * Used to indicate that logo image file is missing or couldn't be loaded
 */
void renderErrorPlaceholder(int fbWidth, int fbHeight) {
    /**
     * Set up viewport and projection matrix
     * Viewport matches framebuffer size
     * Orthographic projection with origin at bottom-left
     */
    glViewport(0, 0, fbWidth, fbHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, fbWidth, 0.0, fbHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    /**
     * Draw red rectangle in center quarter of screen
     * This visually indicates an error condition
     * Red is a standard error color that stands out
     */
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(fbWidth * 0.25f, fbHeight * 0.25f);
        glVertex2f(fbWidth * 0.75f, fbHeight * 0.25f);
        glVertex2f(fbWidth * 0.75f, fbHeight * 0.75f);
        glVertex2f(fbWidth * 0.25f, fbHeight * 0.75f);
    glEnd();
}
