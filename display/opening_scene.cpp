#include "opening_scene.h"
#include "scene.h"
#include <iostream>
#include <stdexcept>
#include <exception>

/**
 * Load the opening scene from JSON file
 * Attempts to load scenes/opening.scene.json and populates the scene structure
 * Handles file errors and parsing errors gracefully without crashing
 */
bool loadOpeningScene(Scene& openingScene) {
    std::cout << "[DEBUG] Loading opening scene..." << std::endl;
    
    /**
     * Attempt to load scene from JSON file
     * The loadScene function handles file I/O and JSON parsing
     * Returns false if file doesn't exist or contains invalid JSON
     */
    try {
        bool loaded = loadScene("scenes/opening.scene.json", openingScene);
        std::cout << "[DEBUG] Scene loaded: " << (loaded ? "yes" : "no") << std::endl;
        
        /**
         * If scene failed to load, log a warning but don't crash
         * The application will continue running, just without the scene
         * This allows graceful degradation when scene file is missing
         */
        if (!loaded) {
            std::cerr << "[WARNING] Failed to load opening scene" << std::endl;
        }
        return loaded;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during scene loading: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during scene loading" << std::endl;
        return false;
    }
}
