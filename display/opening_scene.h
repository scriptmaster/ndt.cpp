#ifndef OPENING_SCENE_H
#define OPENING_SCENE_H

#include "scene.h"

/**
 * Load the opening scene from JSON file
 * Attempts to load scenes/opening.scene.json and populates the scene structure
 * Handles file errors and parsing errors gracefully without crashing
 * 
 * @param openingScene Reference to Scene structure to populate with loaded data
 * @return true if scene loaded successfully, false otherwise
 */
bool loadOpeningScene(Scene& openingScene);

#endif // OPENING_SCENE_H
