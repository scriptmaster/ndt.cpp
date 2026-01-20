#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include "WindowData.h"
#include <vector>

/**
 * WindowManager - Window creation and management
 * 
 * Single responsibility: Create and cleanup GLFW windows
 * Ported from display/window.cpp
 */

// Window management functions
std::vector<WindowData> createWindows();
void cleanupWindows(std::vector<WindowData>& windows);

// Callbacks
void error_callback(int error, const char* description);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void window_focus_callback(GLFWwindow* window, int focused);
void window_iconify_callback(GLFWwindow* window, int iconified);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Window focus/visibility management
void ensureWindowVisible(GLFWwindow* window, bool isPrimary);
void ensurePrimaryWindowFocused(GLFWwindow* window);

#endif // WINDOWMANAGER_H
