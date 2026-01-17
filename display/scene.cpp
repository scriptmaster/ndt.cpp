#include "scene.h"
#include "audio.h"
#include <cstdio>  // For FILE, fopen, fclose, fgets, feof
#include <sstream>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <stdexcept>
#include <exception>

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <iostream>

// Simple JSON parsing helpers
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

static std::string extractStringValue(const std::string& line) {
    size_t colon = line.find(':');
    if (colon == std::string::npos) return "";
    size_t startQuote = line.find('"', colon);
    if (startQuote == std::string::npos) return "";
    size_t start = startQuote + 1;
    if (start >= line.length()) return "";
    size_t end = line.find('"', start);
    if (end == std::string::npos) return "";
    if (end <= start) return "";
    return line.substr(start, end - start);
}

static int extractIntValue(const std::string& line) {
    size_t colon = line.find(':');
    if (colon == std::string::npos) return 0;
    std::string value = trim(line.substr(colon + 1));
    value.erase(std::remove(value.begin(), value.end(), ','), value.end());
    if (value.empty()) return 0;
    try {
        return std::stoi(value);
    } catch (...) {
        return 0;
    }
}

static float extractFloatValue(const std::string& line) {
    size_t colon = line.find(':');
    if (colon == std::string::npos) return 0.0f;
    std::string value = trim(line.substr(colon + 1));
    value.erase(std::remove(value.begin(), value.end(), ','), value.end());
    if (value.empty()) return 0.0f;
    try {
        return std::stof(value);
    } catch (...) {
        return 0.0f;
    }
}

// Background graphics state
static struct {
    struct Triangle {
        float x, y, vx, vy;
        float size;
        float rotation;
        float rotSpeed;
    } triangles[100];
    
    struct Dot {
        float x, y, vx, vy;
    } dots[200];
    
    struct Orb {
        float x, y, vx, vy;
        float r, g, b;
        float radius;
    } orbs[10];
    
    bool initialized = false;
} bgState;

static void initBackgroundGraphics(int width, int height) {
    if (bgState.initialized) return;
    if (width <= 0 || height <= 0) return; // Safety check
    
    // Ensure rand() is seeded (may not be initialized yet)
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

static void renderTriangles(int width, int height, float deltaTime) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.6f, 0.7f, 0.9f, 0.3f);
    
    for (int i = 0; i < 100; i++) {
        auto& t = bgState.triangles[i];
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

static void renderDotsWithLines(int width, int height, float deltaTime, float connectionRange = 100.0f) {
    // Update dots
    for (int i = 0; i < 200; i++) {
        auto& d = bgState.dots[i];
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
            float dx = bgState.dots[i].x - bgState.dots[j].x;
            float dy = bgState.dots[i].y - bgState.dots[j].y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist < connectionRange) {
                float alpha = 1.0f - (dist / connectionRange);
                glColor4f(0.5f, 0.6f, 0.8f, alpha * 0.3f);
                glVertex2f(bgState.dots[i].x, bgState.dots[i].y);
                glVertex2f(bgState.dots[j].x, bgState.dots[j].y);
            }
        }
    }
    glEnd();
    
    // Draw dots
    glPointSize(2.0f);
    glColor4f(0.7f, 0.8f, 1.0f, 0.8f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 200; i++) {
        glVertex2f(bgState.dots[i].x, bgState.dots[i].y);
    }
    glEnd();
    
    glDisable(GL_BLEND);
}

static void renderBlurredOrbs(int width, int height, float deltaTime) {
    // Update orbs
    for (int i = 0; i < 10; i++) {
        auto& o = bgState.orbs[i];
        float oldX = o.x;
        float oldY = o.y;
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
    
    // FIRST: Render a single full-screen linear gradient from top-left to bottom-right
    // Using light Apple-style colors (soft pastels, light greens, soft blues)
    // Gradient extends across the entire screen
    {
        // Fixed angle: top-left to bottom-right (diagonal, ~135 degrees from horizontal)
        float angleRad = 135.0f * 3.14159f / 180.0f; // 135 degrees = top-left to bottom-right
        
        float cosAngle = cosf(angleRad);
        float sinAngle = sinf(angleRad);
        
        // Start position: top-left corner (offset slightly for smoother fade)
        float startX = -width * 0.2f; // Start slightly off-screen top-left
        float startY = height * 1.2f; // Start slightly off-screen top
        
        // Calculate perpendicular direction for gradient width (full screen height)
        float perpX = -sinAngle;
        float perpY = cosAngle;
        float gradientWidth = sqrtf(width * width + height * height); // Full screen diagonal for width
        
        // Gradient length: enough to cover from top-left to bottom-right
        float gradientLength = sqrtf(width * width + height * height) * 1.4f;
        int gradientSteps = 256; // 256 steps for smooth transition
        
        // Light Apple-style colors: soft pastels
        // Start color (top-left): Light mint/soft green (#E8F5E9 or similar)
        float startR = 0.91f; // Light green/mint
        float startG = 0.96f;
        float startB = 0.91f;
        
        // End color (bottom-right): Soft lavender/light purple (#F3E5F5 or similar)
        float endR = 0.95f; // Soft lavender
        float endG = 0.90f;
        float endB = 0.96f;
        
        // Draw linear gradient from top-left to bottom-right with 256 steps
        glBegin(GL_QUAD_STRIP);
        for (int step = 0; step <= gradientSteps; step++) {
            float t = (float)step / gradientSteps; // 0 at start, 1 at end
            
            // Distance from start position (top-left) along gradient direction
            float distFromStart = gradientLength * t;
            
            // Calculate position along gradient direction (from top-left toward bottom-right)
            float gradX = startX + cosAngle * distFromStart;
            float gradY = startY + sinAngle * distFromStart;
            
            // Interpolate color from start (light mint) to end (soft lavender)
            float r = startR + (endR - startR) * t;
            float g = startG + (endG - startG) * t;
            float b = startB + (endB - startB) * t;
            
            // Opacity: light gradient fade (35% at center, fading to edges)
            // Smooth falloff for subtle Apple-style gradient
            float centerT = 0.5f; // Center of gradient
            float distFromCenter = std::abs(t - centerT) * 2.0f; // 0 at center, 1 at edges
            float maxOpacity = 0.35f; // 35% opacity for light Apple gradient
            float alpha = maxOpacity * (1.0f - distFromCenter * distFromCenter); // Quadratic falloff from center
            
            glColor4f(r, g, b, alpha);
            
            // Create full-width gradient strip perpendicular to gradient direction
            float offsetX = perpX * gradientWidth * 0.5f;
            float offsetY = perpY * gradientWidth * 0.5f;
            
            // Each step contributes two vertices: top and bottom edge of the strip
            glVertex2f(gradX + offsetX, gradY + offsetY); // Top edge
            glVertex2f(gradX - offsetX, gradY - offsetY); // Bottom edge
        }
        glEnd();
    }
    
    // SECOND: Render circular orbs with Gaussian blur (like Photoshop)
    // Gaussian blur uses: opacity = e^(-(r²)/(2σ²)) where σ controls blur radius
    // This creates a natural, smooth blur that Photoshop uses
    for (int i = 0; i < 10; i++) {
        auto& o = bgState.orbs[i];
        
        // Gaussian blur parameters
        float sigma = o.radius * 0.5f; // Standard deviation - controls blur spread (50% of radius for smoother blur)
        float maxOpacity = 0.25f; // Maximum opacity at center (25% = reduced from 50% for softer look)
        
        // Draw Gaussian blur using multiple concentric circles
        // More layers = smoother blur, more segments = smoother circle edges
        int layers = 80; // More layers for smoother Gaussian blur (increased from 60)
        int segments = 180; // More segments for smoother circle edges (increased from 128)
        
        for (int layer = 0; layer < layers; layer++) {
            float t = (float)layer / layers; // 0 at center, 1 at edge
            float radius = o.radius * t; // Linear radius increase
            
            // Gaussian function: e^(-(r²)/(2σ²))
            // This is how Photoshop does blur - natural bell curve distribution
            float rSquared = radius * radius;
            float twoSigmaSquared = 2.0f * sigma * sigma;
            float gaussian = expf(-rSquared / twoSigmaSquared); // Gaussian bell curve
            
            // Opacity follows Gaussian distribution
            float alpha = maxOpacity * gaussian;
            
            // Stop rendering when opacity becomes negligible (almost zero)
            if (alpha < 0.001f) {
                break; // Skip remaining layers for performance
            }
            
            glColor4f(o.r, o.g, o.b, alpha);
            
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(o.x, o.y); // Center vertex
            for (int j = 0; j <= segments; j++) {
                float angle = (float)j / segments * 3.14159f * 2.0f;
                glVertex2f(o.x + cosf(angle) * radius, o.y + sinf(angle) * radius);
            }
            glEnd();
        }
    }
    
    glDisable(GL_BLEND);
}

// Parse color string - supports hex format (#ffffff or ffffff) or comma format (r,g,b)
static void parseColor(const std::string& colorStr, float& r, float& g, float& b) {
    r = g = b = 0.1f;
    
    // Check if empty string
    if (colorStr.empty()) return;
    
    // Check if hex format (#ffffff or ffffff)
    std::string hexStr = colorStr;
    if (hexStr[0] == '#') {
        hexStr = hexStr.substr(1); // Remove #
    }
    
    // Try to parse as hex (6 hex digits)
    if (hexStr.length() == 6) {
        bool isHex = true;
        for (char c : hexStr) {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                isHex = false;
                break;
            }
        }
        
        if (isHex) {
            // Parse hex: RRGGBB
            unsigned int hexValue = std::stoul(hexStr, nullptr, 16);
            r = ((hexValue >> 16) & 0xFF) / 255.0f; // Red
            g = ((hexValue >> 8) & 0xFF) / 255.0f;  // Green
            b = (hexValue & 0xFF) / 255.0f;         // Blue
            return;
        }
    }
    
    // Fallback: try comma-separated format "r,g,b"
    size_t comma1 = colorStr.find(',');
    size_t comma2 = colorStr.find(',', comma1 + 1);
    if (comma1 != std::string::npos && comma2 != std::string::npos) {
        try {
            r = std::stof(colorStr.substr(0, comma1));
            g = std::stof(colorStr.substr(comma1 + 1, comma2 - comma1 - 1));
            b = std::stof(colorStr.substr(comma2 + 1));
            // Normalize if values are > 1.0 (assumed 0-255 range)
            if (r > 1.0f || g > 1.0f || b > 1.0f) {
                r /= 255.0f;
                g /= 255.0f;
                b /= 255.0f;
            }
        } catch (...) {
            // Invalid format, use defaults
            r = g = b = 0.1f;
        }
    }
}

bool loadScene(const std::string& filename, Scene& scene) {
    std::cout << "[DEBUG] loadScene: Opening file: " << filename << std::endl;
    std::cout << "[DEBUG] loadScene: Filename length: " << filename.length() << std::endl;
    std::cout << "[DEBUG] loadScene: Filename c_str: " << filename.c_str() << std::endl;
    
    // Initialize scene with defaults
    std::cout << "[DEBUG] loadScene: Initializing scene with defaults..." << std::endl;
    try {
        scene.id = "";
        scene.layout = "grid";
        scene.cols = 8;
        scene.rows = 12;
        scene.bg.image = "";
        scene.bg.color = "";
        scene.bg.graphic = "";
        scene.widgets.clear();
        scene.waveform = true;
        std::cout << "[DEBUG] loadScene: Scene initialized, default waveform: true" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DEBUG] loadScene: Exception during scene initialization: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[DEBUG] loadScene: Unknown exception during scene initialization" << std::endl;
        return false;
    }
    
    /**
     * Create file stream for reading scene JSON file
     * Use try-catch to handle any exceptions during file operations
     * Log each step to help debug file I/O issues
     */
    /**
     * Use C file I/O instead of std::ifstream to avoid crashes
     * std::ifstream was causing segmentation faults on Windows
     * C file I/O (fopen/fgets) is more stable and reliable
     */
    if (filename.empty()) {
        std::cerr << "[ERROR] loadScene: Filename is empty" << std::endl;
        return false;
    }
    
    std::cout << "[DEBUG] loadScene: Opening file with fopen: " << filename << std::endl;
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        std::cerr << "[ERROR] loadScene: Failed to open scene file: " << filename << std::endl;
        return false;
    }
    std::cout << "[DEBUG] loadScene: File opened successfully with fopen" << std::endl;
    
    // Default waveform to true
    scene.waveform = true;
    std::cout << "[DEBUG] loadScene: Waveform set to true" << std::endl;
    
    std::string line;
    std::string currentSection;
    Widget currentWidget;
    bool inWidgets = false;
    bool inBg = false;
    
    /**
     * Read file line by line using C file I/O
     * Use fgets to read lines, similar to std::getline but using C FILE* API
     * Buffer size is 2048 characters, sufficient for JSON lines
     */
    int lineCount = 0;
    char buffer[2048];
    std::cout << "[DEBUG] loadScene: Starting to read file line by line with fgets..." << std::endl;
    try {
        std::cout << "[DEBUG] loadScene: Entering fgets loop..." << std::endl;
        while (fgets(buffer, sizeof(buffer), file) != nullptr) {
            lineCount++;
            /**
             * Convert C string to std::string for processing
             * Remove trailing newline character if present
             * fgets includes the newline in the buffer
             */
            line = std::string(buffer);
            // Remove trailing newline/carriage return
            if (!line.empty() && line.back() == '\n') {
                line.pop_back();
            }
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            std::cout << "[DEBUG] loadScene: Line " << lineCount << " (before trim): [" << line << "]" << std::endl;
            line = trim(line);
            std::cout << "[DEBUG] loadScene: Line " << lineCount << " (after trim): [" << line << "]" << std::endl;
            if (line.empty() || line[0] == '{' || line[0] == '}') {
                std::cout << "[DEBUG] loadScene: Line " << lineCount << " skipped (empty or brace)" << std::endl;
                continue;
            }
            
                std::cout << "[DEBUG] loadScene: Line " << lineCount << " processing..." << std::endl;
            if (line.find("\"id\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found 'id' field" << std::endl;
                scene.id = extractStringValue(line);
                std::cout << "[DEBUG] loadScene: Set id to: [" << scene.id << "]" << std::endl;
            } else if (line.find("\"layout\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found 'layout' field" << std::endl;
                scene.layout = extractStringValue(line);
                std::cout << "[DEBUG] loadScene: Set layout to: [" << scene.layout << "]" << std::endl;
            } else if (line.find("\"cols\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found 'cols' field" << std::endl;
                scene.cols = extractIntValue(line);
                std::cout << "[DEBUG] loadScene: Set cols to: " << scene.cols << std::endl;
            } else if (line.find("\"rows\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found 'rows' field" << std::endl;
                scene.rows = extractIntValue(line);
                std::cout << "[DEBUG] loadScene: Set rows to: " << scene.rows << std::endl;
            } else if (line.find("\"waveform\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found 'waveform' field" << std::endl;
                std::string waveformStr = extractStringValue(line);
                scene.waveform = (waveformStr == "true" || waveformStr == "1");
                std::cout << "[DEBUG] loadScene: Set waveform to: " << (scene.waveform ? "true" : "false") << std::endl;
            } else if (line.find("\"bg\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found 'bg' field, setting inBg=true" << std::endl;
                inBg = true;
            } else if (inBg && line.find("\"image\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found 'image' field in bg" << std::endl;
                scene.bg.image = extractStringValue(line);
                std::cout << "[DEBUG] loadScene: Set bg.image to: [" << scene.bg.image << "]" << std::endl;
            } else if (inBg && line.find("\"color\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found 'color' field in bg" << std::endl;
                scene.bg.color = extractStringValue(line);
                std::cout << "[DEBUG] loadScene: Set bg.color to: [" << scene.bg.color << "]" << std::endl;
            } else if (inBg && line.find("\"graphic\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found 'graphic' field in bg" << std::endl;
                scene.bg.graphic = extractStringValue(line);
                std::cout << "[DEBUG] loadScene: Set bg.graphic to: [" << scene.bg.graphic << "], setting inBg=false" << std::endl;
                inBg = false;
            } else if (line.find("\"widgets\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found 'widgets' field, setting inWidgets=true" << std::endl;
                inWidgets = true;
            } else if (inWidgets && line.find("{") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found widget start '{', initializing currentWidget" << std::endl;
                currentWidget = Widget();
            } else if (inWidgets && line.find("}") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found widget end '}'" << std::endl;
                if (currentWidget.type != "") {
                    std::cout << "[DEBUG] loadScene: Adding widget type: [" << currentWidget.type << "]" << std::endl;
                    scene.widgets.push_back(currentWidget);
                    std::cout << "[DEBUG] loadScene: Widget added, total widgets: " << scene.widgets.size() << std::endl;
                } else {
                    std::cout << "[DEBUG] loadScene: Widget has no type, skipping" << std::endl;
                }
            } else if (inWidgets && line.find("\"type\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found widget 'type' field" << std::endl;
                currentWidget.type = extractStringValue(line);
                std::cout << "[DEBUG] loadScene: Set widget type to: [" << currentWidget.type << "]" << std::endl;
            } else if (inWidgets && line.find("\"language\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found widget 'language' field" << std::endl;
                currentWidget.properties["language"] = extractStringValue(line);
                std::cout << "[DEBUG] loadScene: Set widget language to: [" << currentWidget.properties["language"] << "]" << std::endl;
            } else if (inWidgets && line.find("\"row\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found widget 'row' field" << std::endl;
                currentWidget.row = extractIntValue(line);
                std::cout << "[DEBUG] loadScene: Set widget row to: " << currentWidget.row << std::endl;
            } else if (inWidgets && line.find("\"col\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found widget 'col' field" << std::endl;
                currentWidget.col = extractIntValue(line);
                std::cout << "[DEBUG] loadScene: Set widget col to: " << currentWidget.col << std::endl;
            } else if (inWidgets && line.find("\"width\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found widget 'width' field" << std::endl;
                currentWidget.width = extractIntValue(line);
                std::cout << "[DEBUG] loadScene: Set widget width to: " << currentWidget.width << std::endl;
            } else if (inWidgets && line.find("\"height\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found widget 'height' field" << std::endl;
                currentWidget.height = extractIntValue(line);
                std::cout << "[DEBUG] loadScene: Set widget height to: " << currentWidget.height << std::endl;
            } else if (inWidgets && line.find("\"margin\"") != std::string::npos) {
                std::cout << "[DEBUG] loadScene: Found widget 'margin' field" << std::endl;
                currentWidget.margin = extractFloatValue(line);
                std::cout << "[DEBUG] loadScene: Set widget margin to: " << currentWidget.margin << std::endl;
            } else {
                std::cout << "[DEBUG] loadScene: Line " << lineCount << " did not match any known field, ignoring" << std::endl;
            }
        }
        std::cout << "[DEBUG] loadScene: Exited fgets loop" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] loadScene: Exception during file reading: " << e.what() << std::endl;
        fclose(file);
        return false;
    } catch (...) {
        std::cerr << "[ERROR] loadScene: Unknown exception during file reading" << std::endl;
        fclose(file);
        return false;
    }
    
    /**
     * Close file after reading is complete
     * Always close C FILE* handles to prevent resource leaks
     */
    std::cout << "[DEBUG] loadScene: Finished reading file, closing..." << std::endl;
    fclose(file);
    std::cout << "[DEBUG] loadScene: File closed with fclose" << std::endl;
    
    std::cout << "[DEBUG] loadScene: Parsed " << lineCount << " lines, widgets: " << scene.widgets.size() << std::endl;
    std::cout << "[DEBUG] loadScene: Scene ID: [" << scene.id << "], Layout: [" << scene.layout << "]" << std::endl;
    std::cout << "[DEBUG] loadScene: Grid: " << scene.cols << "x" << scene.rows << std::endl;
    std::cout << "[DEBUG] loadScene: Background - image: [" << scene.bg.image << "], color: [" << scene.bg.color << "], graphic: [" << scene.bg.graphic << "]" << std::endl;
    std::cout << "[DEBUG] loadScene: Waveform enabled: " << (scene.waveform ? "true" : "false") << std::endl;
    std::cout << "[DEBUG] loadScene: Returning true" << std::endl;
    
    return true;
}

void renderScene(const Scene& scene, int windowWidth, int windowHeight, float deltaTime) {
    try {
        // Calculate grid cell size
        if (scene.cols <= 0 || scene.rows <= 0 || windowWidth <= 0 || windowHeight <= 0) {
            std::cerr << "[ERROR] renderScene: Invalid dimensions - cols: " << scene.cols 
                      << ", rows: " << scene.rows << ", width: " << windowWidth 
                      << ", height: " << windowHeight << std::endl;
            return;
        }
        
        float cellWidth = (float)windowWidth / scene.cols;
        float cellHeight = (float)windowHeight / scene.rows;
    
    // Set up viewport
    glViewport(0, 0, windowWidth, windowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Render background color - white by default
    float bgR = 1.0f, bgG = 1.0f, bgB = 1.0f; // White background by default
    if (!scene.bg.color.empty()) {
        parseColor(scene.bg.color, bgR, bgG, bgB);
    }
    glClearColor(bgR, bgG, bgB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Initialize background graphics if needed
    std::cout << "[DEBUG] renderScene: Initializing background graphics..." << std::endl;
    initBackgroundGraphics(windowWidth, windowHeight);
    std::cout << "[DEBUG] renderScene: Background graphics initialized" << std::endl;
    
    // Render background graphic procedure
    std::cout << "[DEBUG] renderScene: Background graphic type: " << scene.bg.graphic << std::endl;
    if (scene.bg.graphic == "triangles") {
        std::cout << "[DEBUG] renderScene: Rendering triangles..." << std::endl;
        renderTriangles(windowWidth, windowHeight, deltaTime);
    } else if (scene.bg.graphic == "dots_lines") {
        renderDotsWithLines(windowWidth, windowHeight, deltaTime);
    } else if (scene.bg.graphic == "blurred_orbs") {
        renderBlurredOrbs(windowWidth, windowHeight, deltaTime);
    }
    
    // Render widgets
    std::cout << "[DEBUG] renderScene: Rendering " << scene.widgets.size() << " widgets..." << std::endl;
    for (const auto& widget : scene.widgets) {
        std::cout << "[DEBUG] renderScene: Widget type: " << widget.type << std::endl;
        if (widget.type == "language_card") {
            // Calculate widget position and size in pixels
            float x = widget.col * cellWidth;
            float y = (scene.rows - widget.row - widget.height) * cellHeight; // Y is from bottom
            float w = widget.width * cellWidth;
            float h = widget.height * cellHeight;
            
            // Apply margin
            float marginX = w * widget.margin;
            float marginY = h * widget.margin;
            x += marginX;
            y += marginY;
            w -= marginX * 2;
            h -= marginY * 2;
            
            // Draw card background
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.2f, 0.25f, 0.3f, 0.8f);
            glBegin(GL_QUADS);
                glVertex2f(x, y);
                glVertex2f(x + w, y);
                glVertex2f(x + w, y + h);
                glVertex2f(x, y + h);
            glEnd();
            
            // Draw card border
            glColor4f(0.4f, 0.5f, 0.6f, 0.9f);
            glLineWidth(2.0f);
            glBegin(GL_LINE_LOOP);
                glVertex2f(x, y);
                glVertex2f(x + w, y);
                glVertex2f(x + w, y + h);
                glVertex2f(x, y + h);
            glEnd();
            
            // Render text (simplified - using points for "English" or Arabic text)
            std::string lang = widget.properties.count("language") ? widget.properties.at("language") : "";
            if (lang == "English") {
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                // Simplified text rendering - would need proper font rendering
                // For now, draw a simple indicator
                glPointSize(10.0f);
                glBegin(GL_POINTS);
                    glVertex2f(x + w * 0.5f, y + h * 0.5f);
                glEnd();
            } else if (lang == "Arabic") {
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                // Arabic text would be rendered here with proper font
                glPointSize(10.0f);
                glBegin(GL_POINTS);
                    glVertex2f(x + w * 0.5f, y + h * 0.5f);
                glEnd();
            }
            
            glDisable(GL_BLEND);
        }
        }
        
        // Render waveform widget if enabled (updated every 100ms, displayed at 60fps)
        if (scene.waveform) {
            try {
                renderWaveformWidget(windowWidth, windowHeight);
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Exception rendering waveform: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[ERROR] Unknown exception rendering waveform" << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in renderScene: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception in renderScene" << std::endl;
    }
}

// Render waveform widget - vertical bars showing amplitude values
void renderWaveformWidget(int windowWidth, int windowHeight) {
    std::vector<float> amplitudes = getWaveformAmplitudes();
    if (amplitudes.empty()) return;
    
    // Waveform positioned at bottom of screen
    float waveformHeight = windowHeight * 0.15f; // 15% of screen height
    float waveformY = 0.0f;
    float waveformX = 0.0f;
    float barWidth = (float)windowWidth / amplitudes.size();
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw waveform bars
    glColor4f(0.2f, 0.8f, 1.0f, 0.8f); // Cyan color
    for (size_t i = 0; i < amplitudes.size(); i++) {
        float barHeight = amplitudes[i] * waveformHeight;
        float x = waveformX + i * barWidth;
        
        glBegin(GL_QUADS);
            glVertex2f(x, waveformY);
            glVertex2f(x + barWidth * 0.8f, waveformY); // 80% width for spacing
            glVertex2f(x + barWidth * 0.8f, waveformY + barHeight);
            glVertex2f(x, waveformY + barHeight);
        glEnd();
    }
    
    glDisable(GL_BLEND);
}
