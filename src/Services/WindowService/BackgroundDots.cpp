#include "BackgroundGraphics.h"
#include <cmath>

#ifdef _WIN32
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

/**
 * BackgroundDots - Dots and lines rendering
 * 
 * Single responsibility: Render animated dots with connecting lines
 */

void renderDotsWithLines(int width, int height, float deltaTime, float connectionRange) {
    Dot* dots = getBackgroundDots();
    
    // Update dots
    for (int i = 0; i < 200; i++) {
        auto& d = dots[i];
        d.x += d.vx * deltaTime;
        d.y += d.vy * deltaTime;
        
        // Wrap around
        if (d.x < 0) d.x += width;
        if (d.x > width) d.x -= width;
        if (d.y < 0) d.y += height;
        if (d.y > height) d.y -= height;
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw connections
    glLineWidth(1.0f);
    glColor4f(0.5f, 0.6f, 0.8f, 0.2f);
    glBegin(GL_LINES);
    for (int i = 0; i < 200; i++) {
        for (int j = i + 1; j < 200; j++) {
            float dx = dots[i].x - dots[j].x;
            float dy = dots[i].y - dots[j].y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist < connectionRange) {
                float alpha = 1.0f - (dist / connectionRange);
                glColor4f(0.5f, 0.6f, 0.8f, alpha * 0.3f);
                glVertex2f(dots[i].x, dots[i].y);
                glVertex2f(dots[j].x, dots[j].y);
            }
        }
    }
    glEnd();
    
    // Draw dots
    glPointSize(2.0f);
    glColor4f(0.7f, 0.8f, 1.0f, 0.8f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 200; i++) {
        glVertex2f(dots[i].x, dots[i].y);
    }
    glEnd();
    
    glDisable(GL_BLEND);
}
