#include "BackgroundGraphics.h"
#include <cmath>
#include <cstdlib>

#ifdef _WIN32
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

/**
 * BackgroundOrbs - Blurred orbs rendering with gradient
 * 
 * Single responsibility: Render blurred orbs with full-screen gradient
 */

void renderBlurredOrbs(int width, int height, float deltaTime) {
    Orb* orbs = getBackgroundOrbs();
    
    // Update orbs
    for (int i = 0; i < 10; i++) {
        auto& o = orbs[i];
        o.x += o.vx * deltaTime;
        o.y += o.vy * deltaTime;
        
        // If orb reaches opposite corner or goes off screen, restart from original corner
        float corners[4][2] = {
            {0.0f, 0.0f}, {(float)width, 0.0f}, {0.0f, (float)height}, {(float)width, (float)height}
        };
        int cornerIdx = i % 4;
        
        // Check if orb has passed the opposite corner or gone off-screen
        if (o.x < -o.radius || o.x > width + o.radius || 
            o.y < -o.radius || o.y > height + o.radius) {
            // Restart from corner with small random offset
            o.x = corners[cornerIdx][0] + (float)(rand() % 50 - 25);
            o.y = corners[cornerIdx][1] + (float)(rand() % 50 - 25);
            
            // Recalculate velocity toward opposite corner
            float oppositeCorners[4][2] = {
                {(float)width, (float)height}, {0.0f, (float)height}, {(float)width, 0.0f}, {0.0f, 0.0f}
            };
            float dx = oppositeCorners[cornerIdx][0] - o.x;
            float dy = oppositeCorners[cornerIdx][1] - o.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > 0.1f) {
                float speed = (float)(rand() % 30 + 40) * 0.1f;
                o.vx = (dx / dist) * speed;
                o.vy = (dy / dist) * speed;
            }
        }
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Render full-screen linear gradient from top-left to bottom-right
    float angleRad = 135.0f * 3.14159f / 180.0f;
    float cosAngle = cosf(angleRad);
    float sinAngle = sinf(angleRad);
    float startX = -width * 0.2f;
    float startY = height * 1.2f;
    float perpX = -sinAngle;
    float perpY = cosAngle;
    float gradientWidth = sqrtf(width * width + height * height);
    float gradientLength = gradientWidth * 1.4f;
    int gradientSteps = 256;
    
    float startR = 0.91f, startG = 0.96f, startB = 0.91f;
    float endR = 0.95f, endG = 0.90f, endB = 0.96f;
    
    glBegin(GL_QUAD_STRIP);
    for (int step = 0; step <= gradientSteps; step++) {
        float t = (float)step / gradientSteps;
        float distFromStart = gradientLength * t;
        float gradX = startX + cosAngle * distFromStart;
        float gradY = startY + sinAngle * distFromStart;
        float r = startR + (endR - startR) * t;
        float g = startG + (endG - startG) * t;
        float b = startB + (endB - startB) * t;
        float centerT = 0.5f;
        float distFromCenter = std::abs(t - centerT) * 2.0f;
        float maxOpacity = 0.35f;
        float alpha = maxOpacity * (1.0f - distFromCenter * distFromCenter);
        
        glColor4f(r, g, b, alpha);
        float offsetX = perpX * gradientWidth * 0.5f;
        float offsetY = perpY * gradientWidth * 0.5f;
        glVertex2f(gradX + offsetX, gradY + offsetY);
        glVertex2f(gradX - offsetX, gradY - offsetY);
    }
    glEnd();
    
    // Render circular orbs with Gaussian blur
    for (int i = 0; i < 10; i++) {
        auto& o = orbs[i];
        float sigma = o.radius * 0.5f;
        float maxOpacity = 0.25f;
        int layers = 80;
        int segments = 180;
        
        for (int layer = 0; layer < layers; layer++) {
            float t = (float)layer / layers;
            float radius = o.radius * t;
            float rSquared = radius * radius;
            float twoSigmaSquared = 2.0f * sigma * sigma;
            float gaussian = expf(-rSquared / twoSigmaSquared);
            float alpha = maxOpacity * gaussian;
            
            if (alpha < 0.001f) break;
            
            glColor4f(o.r, o.g, o.b, alpha);
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(o.x, o.y);
            for (int j = 0; j <= segments; j++) {
                float angle = (float)j / segments * 3.14159f * 2.0f;
                glVertex2f(o.x + cosf(angle) * radius, o.y + sinf(angle) * radius);
            }
            glEnd();
        }
    }
    
    glDisable(GL_BLEND);
}
