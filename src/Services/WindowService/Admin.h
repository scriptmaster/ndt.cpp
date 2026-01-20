#ifndef ADMIN_H
#define ADMIN_H

#include "WindowData.h"
#include "Scene.h"
#include <string>

/**
 * Admin - Admin mode functions
 * 
 * Single responsibility: Admin scene loading and rendering
 * Ported from display/admin.cpp
 */

// Check if click is in top-right 64x64 area and handle tetra-click detection
bool checkTetraClick(WindowData& wd, double xpos, double ypos, int windowWidth, int windowHeight, double currentTime);

// Render admin mode text indicator
void renderAdminModeText(int windowWidth, int windowHeight);

// Render tetra-click indicator
void renderTetraClickIndicator(int windowWidth, int windowHeight, int clickCount);

// Load admin scene from file
bool loadAdminScene(const std::string& sceneFile, Scene& scene);

// Handle admin click interactions
void handleAdminClick(WindowData& wd, double xpos, double ypos, int windowWidth, int windowHeight, double currentTime);

#endif // ADMIN_H
