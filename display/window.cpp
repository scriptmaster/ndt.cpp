#include "window.h"
#include "texture.h"

#ifdef _WIN32
#include <windows.h>
#include <sddl.h>
#include <GL/gl.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>

// Error callback
void error_callback(int /*error*/, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

// Framebuffer size callback
void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height) {
    glViewport(0, 0, width, height);
}

// Window focus callback - only restore primary window focus
void window_focus_callback(GLFWwindow* window, int focused) {
    std::cout << "[DEBUG] window_focus_callback called, focused: " << focused << std::endl;
    if (!window || focused) return;
    
    // Get WindowData from user pointer
    void* userPtr = glfwGetWindowUserPointer(window);
    if (!userPtr) {
        std::cout << "[DEBUG] window_focus_callback: userPtr is null" << std::endl;
        return; // Safety check - user pointer not set yet
    }
    
    WindowData* wd = static_cast<WindowData*>(userPtr);
    bool isPrimary = wd->isPrimary;
    std::cout << "[DEBUG] window_focus_callback: isPrimary=" << isPrimary << std::endl;
    
    // Only restore focus for primary window
    if (isPrimary) {
        ensurePrimaryWindowFocused(window);
    } else {
        // Secondary window: just restore visibility (no focus)
        ensureWindowVisible(window, false);
    }
}

// Window iconify callback - restore visibility only, focus only if primary
void window_iconify_callback(GLFWwindow* window, int iconified) {
    std::cout << "[DEBUG] window_iconify_callback called, iconified: " << iconified << std::endl;
    if (!window || !iconified) return;
    
    // Get WindowData from user pointer
    void* userPtr = glfwGetWindowUserPointer(window);
    if (!userPtr) {
        std::cout << "[DEBUG] window_iconify_callback: userPtr is null" << std::endl;
        return; // Safety check - user pointer not set yet
    }
    
    WindowData* wd = static_cast<WindowData*>(userPtr);
    bool isPrimary = wd->isPrimary;
    std::cout << "[DEBUG] window_iconify_callback: isPrimary=" << isPrimary << std::endl;
    
    // Restore visibility - focus only if primary
    ensureWindowVisible(window, isPrimary);
}

// Mouse button callback - handle clicks for audio seed change (double-click) and fade-out trigger (any click)
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (!window || button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    
    // Get WindowData from user pointer
    void* userPtr = glfwGetWindowUserPointer(window);
    if (!userPtr) return;
    
    WindowData* wd = static_cast<WindowData*>(userPtr);
    bool isPrimary = wd->isPrimary;
    
    // Get current mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    
    double currentTime = glfwGetTime();
    
    // Find the WindowData for this window to update click state
    // We'll need to access it from main.cpp, so for now just store in a static map
    // or pass through user pointer - let's use a simpler approach: store in WindowData via user pointer
    
    // For double-click detection, we need to track last click time and position
    // This will be handled in main.cpp where we have access to WindowData
    std::cout << "[DEBUG] mouse_button_callback: Click at (" << xpos << ", " << ypos << "), isPrimary=" << isPrimary << std::endl;
}

// Ensure window is visible (and optionally focused if primary)
void ensureWindowVisible(GLFWwindow* window, bool isPrimary) {
    if (!window) return;
    
    // Restore window if minimized
    glfwRestoreWindow(window);
    
    // Show window if hidden
    if (!glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
        glfwShowWindow(window);
    }
    
    // Only focus primary windows
    if (isPrimary) {
        ensurePrimaryWindowFocused(window);
    }
    // Secondary windows: just ensure topmost, no focus
    #ifdef _WIN32
    else {
        HWND hwnd = glfwGetWin32Window(window);
        if (hwnd && !IsWindowVisible(hwnd)) {
            ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        }
        // Ensure topmost without activating
        if (hwnd) {
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }
    #endif
}

// Ensure primary window is focused and on top
void ensurePrimaryWindowFocused(GLFWwindow* window) {
    if (!window) return;
    
    // Restore if minimized
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
        glfwRestoreWindow(window);
    }
    
    // Show if hidden
    if (!glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
        glfwShowWindow(window);
    }
    
    #ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);
    if (hwnd) {
        // Check if window is actually visible
        if (!IsWindowVisible(hwnd)) {
            ShowWindow(hwnd, SW_SHOW);
        }
        
        // Ensure topmost
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
                   SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        
        // Set focus only for primary window
        AllowSetForegroundWindow(ASFW_ANY);
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        SetFocus(hwnd);
        BringWindowToTop(hwnd);
    }
    #endif
    
    // Use GLFW focus as well
    glfwFocusWindow(window);
}

// Create windows for all monitors
std::vector<WindowData> createWindows() {
    std::vector<WindowData> windows;
    
    // Set error callback
    glfwSetErrorCallback(error_callback);
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return windows;
    }
    
    // Get monitors
    int monitorCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    
    if (monitorCount == 0) {
        std::cerr << "No monitors detected" << std::endl;
        glfwTerminate();
        return windows;
    }
    
    std::cout << "Detected " << monitorCount << " monitor(s)" << std::endl;
    
    // Set window hints - make windows fullscreen and topmost
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);  // Remove decorations for fullscreen
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);  // Always on top
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);  // Don't show until ready
    
    // Determine which window should be primary (first horizontal monitor)
    bool primaryAssigned = false;
    
    // Create window for each available monitor
    for (int i = 0; i < monitorCount; i++) {
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
        if (!mode) {
            std::cerr << "Warning: Could not get video mode for monitor " << i << std::endl;
            continue;
        }
        
        int width = mode->width;
        int height = mode->height;
        
        // Determine monitor orientation based on aspect ratio
        bool isVertical = (height > width);
        bool isPrimary = (!isVertical && !primaryAssigned); // First horizontal is primary
        if (isPrimary) {
            primaryAssigned = true;
        }
        
        std::string logoPath = isVertical ? "assets/logo_dark.png" : "assets/logo_light.png";
        std::string monitorType = isVertical ? "Vertical" : (isPrimary ? "Horizontal (Primary)" : "Horizontal");
        
        std::cout << "Monitor " << (i + 1) << ": " << width << "x" << height 
                  << " (" << monitorType << ")" << std::endl;
        
        // Get monitor position
        int xpos, ypos;
        glfwGetMonitorPos(monitors[i], &xpos, &ypos);
        
        // Create fullscreen window on monitor
        GLFWwindow* window = glfwCreateWindow(width, height, "NDT Logo Display", 
                                               monitors[i], nullptr);
        
        if (window) {
            glfwMakeContextCurrent(window);
            
            // Store isPrimary in window user pointer BEFORE setting callbacks
            // This prevents callbacks from accessing uninitialized pointer
            // Note: User pointer will be updated after WindowData is added to vector
            // to point to the actual WindowData
            glfwSetWindowUserPointer(window, nullptr);  // Temporarily null
            
            glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
            glfwSetWindowFocusCallback(window, window_focus_callback);
            glfwSetWindowIconifyCallback(window, window_iconify_callback);
            glfwSetMouseButtonCallback(window, mouse_button_callback);
            
            #ifdef _WIN32
            // On Windows, modify window style based on primary/secondary
            HWND hwnd = glfwGetWin32Window(window);
            if (hwnd) {
                // Get current extended window style
                LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
                
                if (isPrimary) {
                    // Primary window: Show in taskbar, can receive focus
                    exStyle |= WS_EX_APPWINDOW;   // Show in taskbar
                    exStyle &= ~WS_EX_TOOLWINDOW; // Remove tool window style
                    exStyle |= WS_EX_TOPMOST;     // Always on top
                } else {
                    // Secondary window: Hide from taskbar, prevent activation
                    exStyle &= ~WS_EX_APPWINDOW;  // Remove app window style
                    exStyle |= WS_EX_TOOLWINDOW;  // Hide from taskbar
                    exStyle |= WS_EX_TOPMOST;     // Always on top
                    exStyle |= WS_EX_NOACTIVATE; // Prevent activation/focus
                }
                
                // Set the new extended style
                SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
                
                // Get current window style and make it borderless
                LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
                style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
                SetWindowLongPtr(hwnd, GWL_STYLE, style);
                
                // Force window to be topmost and update
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            }
            #endif
            
            // Show window after it's configured
            glfwShowWindow(window);
            
            WindowData wd;
            wd.window = window;
            wd.logoPath = logoPath;
            wd.width = width;
            wd.height = height;
            wd.isValid = false;
            wd.isVertical = isVertical;
            wd.isPrimary = isPrimary;
            wd.fadeStartTime = glfwGetTime();  // Record start time
            wd.stateStartTime = wd.fadeStartTime;
            /**
             * Start with logo immediately visible (skip fade-in)
             * Logo shows instantly, and scene loading starts on click/touch
             */
            wd.state = DisplayState::LOGO_SHOWING;
            wd.audioSeed = 12345;  // Default seed, will be loaded from config
            wd.clickDetected = false;
            wd.lastClickTime = 0.0;
            wd.lastClickX = 0.0;
            wd.lastClickY = 0.0;
            wd.isAdmin = false; // Will be set in main.cpp after including admin.h
            wd.adminModeActive = false;
            wd.adminClickCount = 0;
            wd.adminClickStartTime = 0.0;
            wd.currentAdminScene = "";
            wd.openingScene = nullptr;  // Scene will be loaded lazily when needed
            wd.sceneLoading = false;     // Not loading yet
            wd.sceneLoaded = false;      // Not loaded yet (will load on demand)
            wd.loadingProgress = 0.0f;   // No progress yet
            wd.loadingStatus = "";       // No status message yet
            windows.push_back(wd);
            
            // Store pointer to WindowData in user pointer (after adding to vector)
            // This avoids dynamic allocation of bool*
            glfwSetWindowUserPointer(window, &windows.back());
            
            // Only focus primary window
            if (isPrimary) {
                #ifdef _WIN32
                if (hwnd) {
                    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
                               SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                    AllowSetForegroundWindow(ASFW_ANY);
                    SetForegroundWindow(hwnd);
                    SetActiveWindow(hwnd);
                    SetFocus(hwnd);
                    BringWindowToTop(hwnd);
                }
                #endif
                glfwFocusWindow(window);
            }
            
            std::cout << "Created fullscreen window on Monitor " << (i + 1) 
                      << " using " << logoPath << std::endl;
        } else {
            std::cerr << "Failed to create window for monitor " << (i + 1) << std::endl;
        }
    }
    
    if (windows.empty()) {
        std::cerr << "Failed to create any windows" << std::endl;
        glfwTerminate();
        return windows;
    }
    
    // Load textures for each window
    for (auto& wd : windows) {
        glfwMakeContextCurrent(wd.window);
        TextureInfo texInfo = loadTexture(wd.logoPath.c_str());
        wd.texture = texInfo.id;
        wd.textureWidth = texInfo.width;
        wd.textureHeight = texInfo.height;
        wd.isValid = (wd.texture != 0);
        
        if (!wd.isValid) {
            std::cerr << "Warning: Failed to load texture for " << wd.logoPath << std::endl;
        } else {
            std::cout << "Loaded texture: " << wd.logoPath << " (" << wd.textureWidth 
                      << "x" << wd.textureHeight << ")" << std::endl;
        }
    }
    
    // Ensure all windows are visible and topmost
    // Only primary window receives focus
    for (auto& wd : windows) {
        #ifdef _WIN32
        HWND hwnd = glfwGetWin32Window(wd.window);
        if (hwnd) {
            // Ensure all windows are topmost
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, 
                       SWP_NOMOVE | SWP_NOSIZE | (wd.isPrimary ? SWP_SHOWWINDOW : SWP_NOACTIVATE));
        }
        #endif
        
        // Only focus primary window
        if (wd.isPrimary) {
            glfwFocusWindow(wd.window);
            std::cout << "Primary window focused on monitor" << std::endl;
        }
    }
    
    return windows;
}

// Cleanup windows
void cleanupWindows(std::vector<WindowData>& windows) {
    for (auto& wd : windows) {
        glfwMakeContextCurrent(wd.window);
        if (wd.isValid && wd.texture != 0) {
            glDeleteTextures(1, &wd.texture);
        }
        // Scene memory is automatically cleaned up by unique_ptr
        // No explicit delete needed - RAII handles it
        
        // Clean up user pointer - no longer dynamically allocated
        glfwSetWindowUserPointer(wd.window, nullptr);
        glfwDestroyWindow(wd.window);
    }
    windows.clear();
    glfwTerminate();
}
