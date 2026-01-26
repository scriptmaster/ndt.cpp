#include "Admin.h"
#include "Scene.h"
#include "SceneHelpers.h"
#include <iostream>
#include <cmath>

#ifdef _WIN32
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

/**
 * Admin - Admin mode functions implementation
 * 
 * Single responsibility: Admin scene loading and rendering
 * Ported from display/admin.cpp
 */

bool checkTetraClick(WindowData& wd, double xpos, double ypos, int windowWidth, int windowHeight, double currentTime) {
    (void)windowHeight;
    if (!wd.isAdmin) return false;
    
    double topRightX = windowWidth - 64.0;
    double topRightY = 0.0;
    
    if (xpos >= topRightX && xpos <= windowWidth && ypos >= topRightY && ypos <= 64.0) {
        const double TETRA_CLICK_TIME_WINDOW = 2.0;
        
        if (currentTime - wd.lastTetraClickTime > TETRA_CLICK_TIME_WINDOW) {
            wd.tetraClickCount = 0;
        }
        
        wd.tetraClickCount++;
        wd.lastTetraClickTime = currentTime;
        
        // Changed from 4 clicks to 3 clicks (triple-click)
        if (wd.tetraClickCount >= 3) {
            wd.tetraClickCount = 0;
            wd.state = DisplayState::ADMIN_SCENE;
            wd.currentAdminScene = resolveScenePath(
                "config/scenes/admin.scene.json",
                "scenes/admin.scene.json"
            );
            return true;
        }
    }
    return false;
}

void renderAdminModeText(int windowWidth, int windowHeight) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 0.0f, 0.0f, 0.8f);
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

void renderTetraClickIndicator(int windowWidth, int windowHeight, int clickCount) {
    if (clickCount <= 0 || clickCount > 3) return;
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float x = windowWidth - 64.0f;
    float y = windowHeight - 64.0f;
    float barHeight = 12.0f;
    float spacing = 4.0f;
    
    for (int i = 0; i < clickCount; i++) {
        float alpha = 0.3f + (float)i * 0.25f;
        glColor4f(0.2f, 0.8f, 1.0f, alpha);
        float barY = y + (64.0f - barHeight * 3 - spacing * 2) + i * (barHeight + spacing);
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
}

bool loadAdminScene(const std::string& sceneFile, Scene& scene) {
    return loadScene(sceneFile, scene);
}

void handleAdminClick(WindowData& wd, double xpos, double ypos, int windowWidth, int windowHeight, double currentTime) {
    // Placeholder - handle admin widget clicks
    (void)wd;
    (void)xpos;
    (void)ypos;
    (void)windowWidth;
    (void)windowHeight;
    (void)currentTime;
}
