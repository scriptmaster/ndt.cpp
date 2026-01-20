#ifndef BACKGROUND_GRAPHICS_H
#define BACKGROUND_GRAPHICS_H

/**
 * BackgroundGraphics - Background graphics state and rendering
 * 
 * Single responsibility: Procedural background graphics (triangles, dots, orbs)
 * Ported from display/scene.cpp
 */

// Initialize background graphics state
void initBackgroundGraphics(int width, int height);

// Render different background graphic types
void renderTriangles(int width, int height, float deltaTime);
void renderDotsWithLines(int width, int height, float deltaTime, float connectionRange = 100.0f);
void renderBlurredOrbs(int width, int height, float deltaTime);

// Internal state accessors (for split rendering files)
struct Triangle { float x, y, vx, vy; float size; float rotation; float rotSpeed; };
struct Dot { float x, y, vx, vy; };
struct Orb { float x, y, vx, vy; float r, g, b; float radius; };

Triangle* getBackgroundTriangles();
Dot* getBackgroundDots();
Orb* getBackgroundOrbs();

#endif // BACKGROUND_GRAPHICS_H
