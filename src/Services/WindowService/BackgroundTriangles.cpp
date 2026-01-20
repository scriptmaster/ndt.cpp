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
 * BackgroundTriangles - Triangle rendering
 * 
 * Single responsibility: Render animated triangles background
 */

void renderTriangles(int width, int height, float deltaTime) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.6f, 0.7f, 0.9f, 0.3f);
    
    Triangle* triangles = getBackgroundTriangles();
    
    for (int i = 0; i < 100; i++) {
        auto& t = triangles[i];
        t.x += t.vx * deltaTime;
        t.y += t.vy * deltaTime;
        t.rotation += t.rotSpeed * deltaTime;
        
        // Wrap around
        if (t.x < 0) t.x += width;
        if (t.x > width) t.x -= width;
        if (t.y < 0) t.y += height;
        if (t.y > height) t.y -= height;
        
        // Draw triangle
        glPushMatrix();
        glTranslatef(t.x, t.y, 0);
        glRotatef(t.rotation, 0, 0, 1);
        glBegin(GL_TRIANGLES);
            glVertex2f(0, t.size);
            glVertex2f(-t.size * 0.866f, -t.size * 0.5f);
            glVertex2f(t.size * 0.866f, -t.size * 0.5f);
        glEnd();
        glPopMatrix();
    }
    
    glDisable(GL_BLEND);
}
