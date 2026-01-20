#ifndef SCENE_HELPERS_H
#define SCENE_HELPERS_H

#include <string>

/**
 * SceneHelpers - Helper functions for scene parsing and rendering
 * 
 * Single responsibility: JSON parsing helpers and color parsing
 * Ported from display/scene.cpp
 */

// Simple JSON parsing helpers
std::string trim(const std::string& str);
std::string extractStringValue(const std::string& line);
int extractIntValue(const std::string& line);
float extractFloatValue(const std::string& line);

// Parse color string - supports hex format (#ffffff or ffffff) or comma format (r,g,b)
void parseColor(const std::string& colorStr, float& r, float& g, float& b);

// Resolve scene file path with a legacy fallback path
std::string resolveScenePath(const std::string& preferredPath, const std::string& legacyPath);

#endif // SCENE_HELPERS_H
