#ifndef SCENE_LOGGER_H
#define SCENE_LOGGER_H

#include <string>

// Scene-specific logging functions
// Logs to scene.log (no timestamps) and limits frequency
void initSceneLogger();
void cleanupSceneLogger();
void logSceneRender(int frameCount, int fbWidth, int fbHeight, int state, float deltaTime, 
                    const std::string& bgGraphic, int widgetCount);

// Audio-specific logging functions
// Logs to audio.log (no timestamps)
void initAudioLogger();
void cleanupAudioLogger();
void logAudio(const std::string& message);

#endif // SCENE_LOGGER_H
