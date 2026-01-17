#ifndef TEXTURE_H
#define TEXTURE_H

struct TextureInfo {
    unsigned int id;
    int width;
    int height;
};

TextureInfo loadTexture(const char* path);
void renderTexture(unsigned int texture, int textureWidth, int textureHeight, 
                  int windowWidth, int windowHeight, float alpha = 1.0f);

#endif // TEXTURE_H
