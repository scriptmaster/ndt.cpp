#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <GL/gl.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

/**
 * UI Test - Standalone test for UI rendering
 * 
 * Tests:
 * 1. Opens a full screen white window (similar to main program)
 * 2. Draws black text at 4 corners with position names
 * 3. Tests basic OpenGL rendering
 */

// Simple text rendering using OpenGL (bitmap style)
void renderText(float x, float y, const std::string& text) {
    // Simple text rendering using points/rectangles
    // In a real implementation, you'd use a font library
    // For now, we'll draw rectangles as placeholders for text
    
    glColor3f(0.0f, 0.0f, 0.0f); // Black
    
    // Draw a rectangle for each character (simplified)
    float charWidth = 20.0f;
    float charHeight = 30.0f;
    
    for (size_t i = 0; i < text.length(); i++) {
        float charX = x + (i * charWidth);
        float charY = y;
        
        // Draw rectangle for character
        glBegin(GL_QUADS);
            glVertex2f(charX, charY);
            glVertex2f(charX + charWidth, charY);
            glVertex2f(charX + charWidth, charY + charHeight);
            glVertex2f(charX, charY + charHeight);
        glEnd();
    }
}

// Better text rendering using raster position and bitmap
void renderTextRaster(float x, float y, const std::string& text) {
    glColor3f(0.0f, 0.0f, 0.0f); // Black
    
    // Use GLUT-style bitmap rendering (if available)
    // Otherwise, use simple rectangle rendering
    glRasterPos2f(x, y);
    
    // For each character, draw a simple representation
    // This is a placeholder - real text rendering would use a font
    float charWidth = 15.0f;
    float charHeight = 25.0f;
    float spacing = 2.0f;
    
    for (size_t i = 0; i < text.length(); i++) {
        float charX = x + (i * (charWidth + spacing));
        
        // Draw filled rectangle for each character
        glBegin(GL_QUADS);
            glVertex2f(charX, y);
            glVertex2f(charX + charWidth, y);
            glVertex2f(charX + charWidth, y + charHeight);
            glVertex2f(charX, y + charHeight);
        glEnd();
    }
}

// Error callback
void error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "UI Test - Full Screen Window" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nThis test will:" << std::endl;
    std::cout << "1. Open a full screen white window" << std::endl;
    std::cout << "2. Draw black text at 4 corners:" << std::endl;
    std::cout << "   - Top Left: 'TOP LEFT'" << std::endl;
    std::cout << "   - Top Right: 'TOP RIGHT'" << std::endl;
    std::cout << "   - Bottom Left: 'BOTTOM LEFT'" << std::endl;
    std::cout << "   - Bottom Right: 'BOTTOM RIGHT'" << std::endl;
    std::cout << "\nPress Enter to start..." << std::endl;
    std::cin.get();
    
    // Initialize GLFW
    glfwSetErrorCallback(error_callback);
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }
    
    // Get primary monitor
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
        std::cerr << "Failed to get primary monitor" << std::endl;
        glfwTerminate();
        return 1;
    }
    
    // Get monitor video mode
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode) {
        std::cerr << "Failed to get video mode" << std::endl;
        glfwTerminate();
        return 1;
    }
    
    int width = mode->width;
    int height = mode->height;
    
    std::cout << "Monitor resolution: " << width << "x" << height << std::endl;
    
    // Create full screen window
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // No window decorations
    
    GLFWwindow* window = glfwCreateWindow(width, height, "UI Test", monitor, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }
    
    // Make context current
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync
    
    // Set up OpenGL viewport
    glViewport(0, 0, width, height);
    
    std::cout << "\nWindow created successfully!" << std::endl;
    std::cout << "Press ESC or close window to exit\n" << std::endl;
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Set up orthographic projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, width, 0.0, height, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // Clear to white
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Disable depth test and enable blending
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Draw text at 4 corners
        float margin = 20.0f;
        float textHeight = 30.0f;
        
        // Top Left
        renderTextRaster(margin, height - textHeight - margin, "TOP LEFT");
        
        // Top Right
        float topRightX = width - margin - (8 * 17.0f); // Approximate width for "TOP RIGHT"
        renderTextRaster(topRightX, height - textHeight - margin, "TOP RIGHT");
        
        // Bottom Left
        renderTextRaster(margin, margin, "BOTTOM LEFT");
        
        // Bottom Right
        float bottomRightX = width - margin - (11 * 17.0f); // Approximate width for "BOTTOM RIGHT"
        renderTextRaster(bottomRightX, margin, "BOTTOM RIGHT");
        
        // Swap buffers
        glfwSwapBuffers(window);
        
        // Poll events
        glfwPollEvents();
        
        // Check for ESC key
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
    
    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "\nUI Test completed successfully!" << std::endl;
    
    return 0;
}
