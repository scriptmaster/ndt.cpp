#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <vector>
#include <map>

struct BackgroundConfig {
    std::string image;
    std::string color;
    std::string graphic;  // "triangles", "dots_lines", "blurred_orbs"
};

struct Widget {
    std::string type;
    std::map<std::string, std::string> properties;
    int row;
    int col;
    int width;
    int height;
    float margin;
};

struct Scene {
    std::string id;
    std::string layout;
    int cols;
    int rows;
    BackgroundConfig bg;
    std::vector<Widget> widgets;
    bool waveform;  // Show waveform widget (default: true)
};

// Scene management
bool loadScene(const std::string& filename, Scene& scene);
void renderScene(const Scene& scene, int windowWidth, int windowHeight, float deltaTime);
void renderWaveformWidget(int windowWidth, int windowHeight); // Waveform widget rendering

#endif // SCENE_H
