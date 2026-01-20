#include "SceneLogger.h"
#include <fstream>
#include <iostream>
#include <sstream>

/**
 * SceneLogger implementation
 * Logs scene rendering and audio events to files
 */

static std::ofstream sceneLogFile;
static std::ofstream audioLogFile;

void initSceneLogger() {
    sceneLogFile.open("logs/scene.log", std::ios::app);
    audioLogFile.open("logs/audio.log", std::ios::app);
}

void cleanupSceneLogger() {
    if (sceneLogFile.is_open()) {
        sceneLogFile.close();
    }
    if (audioLogFile.is_open()) {
        audioLogFile.close();
    }
}

void logSceneRender(int frameCount, int fbWidth, int fbHeight, int state, 
                    float deltaTime, const std::string& bgGraphic, int widgetCount) {
    if (frameCount != 0 && frameCount % 1000 != 0) {
        return;
    }
    
    std::ostringstream line;
    line << "Frame:" << frameCount 
         << " FB:" << fbWidth << "x" << fbHeight
         << " State:" << state
         << " DeltaTime:" << deltaTime
         << " BG:" << bgGraphic
         << " Widgets:" << widgetCount;
    
    if (sceneLogFile.is_open()) {
        sceneLogFile << line.str() << std::endl;
        sceneLogFile.flush();
    }
}

void initAudioLogger() {
    if (!audioLogFile.is_open()) {
        audioLogFile.open("logs/audio.log", std::ios::app);
    }
}

void cleanupAudioLogger() {
    if (audioLogFile.is_open()) {
        audioLogFile.close();
    }
}

void logAudio(const std::string& message) {
    if (audioLogFile.is_open()) {
        audioLogFile << message << std::endl;
        audioLogFile.flush();
    }
}
