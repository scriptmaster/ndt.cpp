#ifndef TEXTURELOADER_H
#define TEXTURELOADER_H

/**
 * TextureLoader - Texture loading and rendering
 * 
 * Single responsibility: Load textures from files and render them
 */

struct TextureInfo {
    unsigned int id;
    int width;
    int height;
};

namespace TextureLoader {
    /**
     * Load texture from image file
     * @param path Path to image file
     * @return TextureInfo with OpenGL texture ID and dimensions
     */
    TextureInfo LoadTexture(const char* path);

    /**
     * Render texture as centered quad
     * Renders at 50% of monitor resolution while maintaining aspect ratio
     * @param texture OpenGL texture ID
     * @param textureWidth Original texture width
     * @param textureHeight Original texture height
     * @param windowWidth Window width in pixels
     * @param windowHeight Window height in pixels
     * @param alpha Alpha value for fade-in effect (1.0 = opaque)
     */
    void RenderTexture(unsigned int texture, int textureWidth, int textureHeight, 
                       int windowWidth, int windowHeight, float alpha = 1.0f);
}

#endif // TEXTURELOADER_H
