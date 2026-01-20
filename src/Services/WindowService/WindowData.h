#ifndef WINDOWDATA_H
#define WINDOWDATA_H

#include <GLFW/glfw3.h>
#include <string>
#include <vector>

/**
 * WindowData - Window state and configuration
 * 
 * Single responsibility: Data structures for window management
 * Defines WindowData struct and DisplayState enum used across WindowService
 */

// Forward declaration (full definition in SceneManager.h)
struct Scene;

enum class DisplayState {
    LOGO_FADE_IN,   // 0.8s fade-in
    LOGO_SHOWING,   // Showing logo at full opacity
    LOGO_FADE_OUT,  // 2s fade-out
    OPENING_SCENE,  // Showing opening scene
    ADMIN_SCENE     // Showing admin scene
};

struct WindowData {
    GLFWwindow* window;
    std::string logoPath;
    int width;
    int height;
    unsigned int texture;
    int textureWidth;   // Original texture width
    int textureHeight;  // Original texture height
    bool isValid;
    bool isVertical;
    bool isPrimary;     // Only primary window can receive focus
    double fadeStartTime;  // Time when fade-in starts (in seconds)
    DisplayState state;    // Current display state
    double stateStartTime; // Time when current state started
    int audioSeed;         // Seed for procedural audio generation
    bool clickDetected;    // True if user clicked to skip 20s wait
    double lastClickTime;  // Time of last click for double-click detection
    double lastClickX;     // X position of last click
    double lastClickY;     // Y position of last click
    bool isAdmin;          // True if running as admin
    bool adminModeActive;  // True if admin mode is active
    int adminClickCount;   // Count of clicks in top-right corner
    double adminClickStartTime; // Time when admin click sequence started
    std::vector<double> adminClickTimes; // Times of admin clicks
    std::vector<std::pair<double, double>> adminClickPositions; // Positions of admin clicks
    int tetraClickCount;   // Count of tetra-clicks (4 clicks in top-right corner)
    double lastTetraClickTime; // Time of last tetra-click
    std::string currentAdminScene; // Current admin scene file
    struct Scene* openingScene;    // Opening scene (loaded lazily, allocated when needed)
    bool sceneLoading;             // True if scene is currently loading
    bool sceneLoaded;              // True if scene was successfully loaded
    float loadingProgress;         // Loading progress (0.0 to 1.0)
    std::string loadingStatus;     // Loading status message
};

#endif // WINDOWDATA_H
