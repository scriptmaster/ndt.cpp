#include "BackgroundGraphics.h"
#include <cmath>
#include <ctime>
#include <cstdlib>

#ifdef _WIN32
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

/**
 * BackgroundGraphics - Background graphics state and initialization
 * 
 * Single responsibility: State management and initialization
 */

// Background graphics state
static struct {
    Triangle triangles[100];
    Dot dots[200];
    Orb orbs[10];
    bool initialized = false;
} bgState;

// Accessors for rendering functions
Triangle* getBackgroundTriangles() { return bgState.triangles; }
Dot* getBackgroundDots() { return bgState.dots; }
Orb* getBackgroundOrbs() { return bgState.orbs; }

void initBackgroundGraphics(int width, int height) {
    if (bgState.initialized) return;
    if (width <= 0 || height <= 0) return;
    
    static bool randSeeded = false;
    if (!randSeeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        randSeeded = true;
    }
    
    // Initialize triangles
    for (int i = 0; i < 100; i++) {
        bgState.triangles[i].x = (float)(rand() % width);
        bgState.triangles[i].y = (float)(rand() % height);
        bgState.triangles[i].vx = (float)(rand() % 20 - 10) * 0.1f;
        bgState.triangles[i].vy = (float)(rand() % 20 - 10) * 0.1f;
        bgState.triangles[i].size = (float)(rand() % 20 + 10);
        bgState.triangles[i].rotation = (float)(rand() % 360);
        bgState.triangles[i].rotSpeed = (float)(rand() % 10 - 5) * 0.5f;
    }
    
    // Initialize dots
    for (int i = 0; i < 200; i++) {
        bgState.dots[i].x = (float)(rand() % width);
        bgState.dots[i].y = (float)(rand() % height);
        bgState.dots[i].vx = (float)(rand() % 30 - 15) * 0.1f;
        bgState.dots[i].vy = (float)(rand() % 30 - 15) * 0.1f;
    }
    
    // Initialize orbs
    for (int i = 0; i < 10; i++) {
        bgState.orbs[i].x = (float)(rand() % width);
        bgState.orbs[i].y = (float)(rand() % height);
        bgState.orbs[i].vx = (float)(rand() % 40 - 20) * 0.1f;
        bgState.orbs[i].vy = (float)(rand() % 40 - 20) * 0.1f;
        bgState.orbs[i].r = (float)(rand() % 100 + 150) / 255.0f;
        bgState.orbs[i].g = (float)(rand() % 100 + 150) / 255.0f;
        bgState.orbs[i].b = (float)(rand() % 100 + 150) / 255.0f;
        bgState.orbs[i].radius = (float)(rand() % 100 + 150);
    }
    
    bgState.initialized = true;
}
