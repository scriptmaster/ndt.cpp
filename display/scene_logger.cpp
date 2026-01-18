#include "scene_logger.h"
#include <fstream>
#include <iostream>
#include <sstream>

static std::ofstream sceneLogFile;
static std::ofstream audioLogFile;

void initSceneLogger() {
    sceneLogFile.open("scene.log", std::ios::app);
    audioLogFile.open("audio.log", std::ios::app);
}

void cleanupSceneLogger() {
    if (sceneLogFile.is_open()) {
        sceneLogFile.close();
    }
    if (audioLogFile.is_open()) {
        audioLogFile.close();
    }
}

void logSceneRender(int frameCount, int fbWidth, int fbHeight, int state, float deltaTime, 
                    const std::string& bgGraphic, int widgetCount) {
    // Only log on frame 0 and every 1000th frame
    if (frameCount != 0 && frameCount % 1000 != 0) {
        return;
    }
    
    // Combine all info into one line
    std::ostringstream line;
    line << "Frame:" << frameCount 
         << " FB:" << fbWidth << "x" << fbHeight
         << " State:" << state
         << " DeltaTime:" << deltaTime
         << " BG:" << bgGraphic
         << " Widgets:" << widgetCount;
    
    if (sceneLogFile.is_open()) {
        sceneLogFile << line.str() << std::endl;
        sceneLogFile.flush(); // Ensure it's written immediately
    }
}

void initAudioLogger() {
    // Audio logger is initialized with scene logger
    if (!audioLogFile.is_open()) {
        audioLogFile.open("audio.log", std::ios::app);
    }
}

void cleanupAudioLogger() {
    // Audio logger is cleaned up with scene logger
    if (audioLogFile.is_open()) {
        audioLogFile.close();
    }
}

void logAudio(const std::string& message) {
    if (audioLogFile.is_open()) {
        audioLogFile << message << std::endl;
        audioLogFile.flush(); // Ensure it's written immediately
    }
}
