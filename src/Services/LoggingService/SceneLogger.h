#ifndef SCENE_LOGGER_H
#define SCENE_LOGGER_H

#include <string>

/**
 * SceneLogger - Scene and audio logging functions
 * 
 * Single responsibility: Log scene rendering and audio events
 * Ported from display/scene_logger.h
 */

// Scene-specific logging functions
void initSceneLogger();
void cleanupSceneLogger();
void logSceneRender(int frameCount, int fbWidth, int fbHeight, int state, 
                    float deltaTime, const std::string& bgGraphic, int widgetCount);

// Audio-specific logging functions
void initAudioLogger();
void cleanupAudioLogger();
void logAudio(const std::string& message);

#endif // SCENE_LOGGER_H
