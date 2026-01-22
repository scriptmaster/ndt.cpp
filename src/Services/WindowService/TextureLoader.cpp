#include "TextureLoader.h"

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

/**
 * TextureLoader implementation
 * 
 * Single responsibility: Load textures from files and render them
 */

namespace TextureLoader {
    TextureInfo LoadTexture(const char* path) {
        TextureInfo info = {0, 0, 0};
        glGenTextures(1, &info.id);
        
        int width, height, nrComponents;
        // Force loading as RGBA to handle transparency correctly
        unsigned char* data = stbi_load(path, &width, &height, &nrComponents, STBI_rgb_alpha);
        
        if (data) {
            info.width = width;
            info.height = height;
            
            // Always use RGBA format for proper alpha channel support
            glBindTexture(GL_TEXTURE_2D, info.id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            
            // Use OpenGL 2.1 compatible texture parameters
            // GL_CLAMP works for OpenGL 2.1 (GL_CLAMP_TO_EDGE requires 1.2+ but may not be in headers)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            stbi_image_free(data);
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
            stbi_image_free(data);
            info.id = 0;
        }
        
        return info;
    }

    void RenderTexture(unsigned int texture, int textureWidth, int textureHeight, 
                       int windowWidth, int windowHeight, float alpha) {
        if (texture == 0 || textureWidth == 0 || textureHeight == 0) return;
        
        // Enable alpha blending for transparency and fade-in
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        glViewport(0, 0, windowWidth, windowHeight);
        
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // Target size: 50% of monitor resolution
        float targetWidth = windowWidth * 0.5f;
        float targetHeight = windowHeight * 0.5f;
        
        // Calculate aspect ratios
        float textureAspect = (float)textureWidth / (float)textureHeight;
        float targetAspect = targetWidth / targetHeight;
        
        // Calculate actual quad size maintaining texture aspect ratio
        float quadWidth, quadHeight;
        if (textureAspect > targetAspect) {
            // Texture is wider - fit to target width
            quadWidth = targetWidth;
            quadHeight = targetWidth / textureAspect;
        } else {
            // Texture is taller - fit to target height
            quadHeight = targetHeight;
            quadWidth = targetHeight * textureAspect;
        }
        
        // Center the quad
        float x = (windowWidth - quadWidth) * 0.5f;
        float y = (windowHeight - quadHeight) * 0.5f;
        
        // Use alpha for fade-in effect
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(x + quadWidth, y);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(x + quadWidth, y + quadHeight);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y + quadHeight);
        glEnd();
        
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
    }
}
