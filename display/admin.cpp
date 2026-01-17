#include "admin.h"
#include "window.h"
#include "scene.h"

#ifdef _WIN32
#include <windows.h>
#include <sddl.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#include <unistd.h>
#endif

#include <iostream>
#include <cmath>
#include <algorithm>

// Check if running as admin (Windows)
bool isRunningAsAdmin() {
    #ifdef _WIN32
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                 &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    return isAdmin == TRUE;
    #else
    return geteuid() == 0; // Unix/Linux: root user
    #endif
}

// Check if click is in top-right 64x64 area and handle tetra-click detection
bool checkTetraClick(WindowData& wd, double xpos, double ypos, int windowWidth, int windowHeight, double currentTime) {
    if (!wd.isAdmin) return false;
    
    // Top-right 64x64 area
    double topRightX = windowWidth - 64.0;
    double topRightY = 0.0;
    
    // Check if click is in top-right 64x64 area
    if (xpos >= topRightX && xpos <= windowWidth && ypos >= topRightY && ypos <= 64.0) {
        // Check if we need to reset the click sequence (too much time passed or outside time window)
        const double TETRA_CLICK_TIME_WINDOW = 2.0; // 1.5-2 seconds
        
        if (wd.adminClickCount == 0 || (currentTime - wd.adminClickStartTime) > TETRA_CLICK_TIME_WINDOW) {
            // Start new sequence
            wd.adminClickCount = 1;
            wd.adminClickStartTime = currentTime;
            wd.adminClickTimes.clear();
            wd.adminClickPositions.clear();
            wd.adminClickTimes.push_back(currentTime);
            wd.adminClickPositions.push_back(std::make_pair(xpos, ypos));
            std::cout << "[DEBUG] Admin click sequence started: 1/4" << std::endl;
        } else {
            // Continue sequence
            wd.adminClickCount++;
            wd.adminClickTimes.push_back(currentTime);
            wd.adminClickPositions.push_back(std::make_pair(xpos, ypos));
            std::cout << "[DEBUG] Admin click: " << wd.adminClickCount << "/4" << std::endl;
            
            // Check if we have 4 clicks within the time window
            if (wd.adminClickCount >= 4 && (currentTime - wd.adminClickStartTime) <= TETRA_CLICK_TIME_WINDOW) {
                // Tetra-click detected! Activate admin mode
                wd.adminModeActive = true;
                wd.state = DisplayState::ADMIN_SCENE;
                wd.currentAdminScene = "scenes/admin.scene.json";
                wd.stateStartTime = currentTime;
                std::cout << "[DEBUG] Admin mode activated!" << std::endl;
                return true;
            }
        }
    } else {
        // Click outside the area - reset sequence
        if (wd.adminClickCount > 0) {
            wd.adminClickCount = 0;
            wd.adminClickStartTime = 0.0;
            wd.adminClickTimes.clear();
            wd.adminClickPositions.clear();
        }
    }
    
    return false;
}

// Render "admin mode" text at bottom-left in red and tetra-click indicator
void renderAdminModeText(int windowWidth, int windowHeight) {
    // Set up 2D rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // This is a placeholder - actual text rendering would need a font library
    // For now, just draw a red rectangle indicator at bottom-left
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 0.0f, 0.0f, 0.8f); // Red
    glBegin(GL_QUADS);
        glVertex2f(10.0f, 10.0f);
        glVertex2f(150.0f, 10.0f);
        glVertex2f(150.0f, 30.0f);
        glVertex2f(10.0f, 30.0f);
    glEnd();
    glDisable(GL_BLEND);
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Render tetra-click indicator in top-right corner
void renderTetraClickIndicator(int windowWidth, int windowHeight, int clickCount) {
    if (clickCount <= 0 || clickCount > 4) return;
    
    // Set up 2D rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw indicator in top-right 64x64 area
    float x = windowWidth - 64.0f;
    float y = windowHeight - 64.0f;
    float size = 64.0f;
    float barHeight = 12.0f;
    float spacing = 4.0f;
    
    // Draw click count bars
    for (int i = 0; i < clickCount; i++) {
        float alpha = 0.3f + (float)i * 0.2f; // Fade in as clicks increase
        glColor4f(0.2f, 0.8f, 1.0f, alpha); // Cyan
        float barY = y + (size - barHeight * 4 - spacing * 3) + i * (barHeight + spacing);
        glBegin(GL_QUADS);
            glVertex2f(x + 10.0f, barY);
            glVertex2f(x + 54.0f, barY);
            glVertex2f(x + 54.0f, barY + barHeight);
            glVertex2f(x + 10.0f, barY + barHeight);
        glEnd();
    }
    
    glDisable(GL_BLEND);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Disable texturing
    glDisable(GL_TEXTURE_2D);
    
    // Enable blending for text
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set red color
    glColor3f(1.0f, 0.0f, 0.0f); // Red
    
    // Simple text rendering using quads (you can improve this with proper font rendering)
    // For now, draw a small rectangle as placeholder - proper text rendering would need font loading
    float textX = 10.0f;
    float textY = 10.0f;
    float textWidth = 120.0f;
    float textHeight = 20.0f;
    
    // Draw red rectangle as placeholder for "admin mode" text
    glBegin(GL_QUADS);
    glVertex2f(textX, textY);
    glVertex2f(textX + textWidth, textY);
    glVertex2f(textX + textWidth, textY + textHeight);
    glVertex2f(textX, textY + textHeight);
    glEnd();
    
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Load admin scene
bool loadAdminScene(const std::string& sceneFile, Scene& scene) {
    return loadScene(sceneFile, scene);
}

// Handle admin click (widget interaction)
void handleAdminClick(WindowData& wd, double xpos, double ypos, int windowWidth, int windowHeight, double currentTime) {
    // This will be called when clicking on admin scene widgets
    // Handle tab switching and control button clicks here
    // For now, placeholder implementation
}
