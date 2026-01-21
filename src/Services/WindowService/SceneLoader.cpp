#include "Scene.h"
#include "SceneHelpers.h"
#include <cstdio>
#include <iostream>
#include <string>

// Forward declarations for dependencies to be ported later
extern void logSceneRender(int frameCount, int fbWidth, int fbHeight, int state, 
                           float deltaTime, const std::string& bgGraphic, int widgetCount);

/**
 * SceneLoader - Scene JSON file loading
 * 
 * Single responsibility: Load scene from JSON file
 */

bool loadScene(const std::string& filename, Scene& scene) {
    std::cout << "[DEBUG] loadScene: Opening file: " << filename << std::endl;
    
    // Initialize scene with defaults
    try {
        scene.id = "";
        scene.layout = "grid";
        scene.cols = 8;
        scene.rows = 12;
        scene.bg.image = "";
        scene.bg.color = "";
        scene.bg.graphic = "";
        scene.widgets.clear();
        scene.waveform = true;
    } catch (const std::exception& e) {
        std::cerr << "[DEBUG] loadScene: Exception during scene initialization: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[DEBUG] loadScene: Unknown exception during scene initialization" << std::endl;
        return false;
    }
    
    if (filename.empty()) {
        std::cerr << "[ERROR] loadScene: Filename is empty" << std::endl;
        return false;
    }
    
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        std::cerr << "[ERROR] loadScene: Failed to open scene file: " << filename << std::endl;
        return false;
    }
    
    scene.waveform = true;
    std::string line;
    Widget currentWidget;
    bool inWidgets = false;
    bool inBg = false;
    
    int lineCount = 0;
    char buffer[2048];
    try {
        while (fgets(buffer, sizeof(buffer), file) != nullptr) {
            lineCount++;
            line = std::string(buffer);
            if (!line.empty() && line.back() == '\n') line.pop_back();
            if (!line.empty() && line.back() == '\r') line.pop_back();
            line = trim(line);
            
            if (line.empty()) continue;
            if ((line[0] == '{' || line[0] == '}') && !inWidgets && !inBg) continue;
            
            if (line.find("\"id\"") != std::string::npos) {
                scene.id = extractStringValue(line);
            } else if (line.find("\"layout\"") != std::string::npos) {
                scene.layout = extractStringValue(line);
            } else if (line.find("\"cols\"") != std::string::npos) {
                scene.cols = extractIntValue(line);
            } else if (line.find("\"rows\"") != std::string::npos) {
                scene.rows = extractIntValue(line);
            } else if (line.find("\"waveform\"") != std::string::npos) {
                std::string waveformStr = extractStringValue(line);
                scene.waveform = (waveformStr == "true" || waveformStr == "1");
            } else if (line.find("\"bg\"") != std::string::npos) {
                inBg = true;
            } else if (inBg && line.find("\"image\"") != std::string::npos) {
                scene.bg.image = extractStringValue(line);
            } else if (inBg && line.find("\"color\"") != std::string::npos) {
                scene.bg.color = extractStringValue(line);
            } else if (inBg && line.find("\"graphic\"") != std::string::npos) {
                scene.bg.graphic = extractStringValue(line);
                inBg = false;
            } else if (line.find("\"widgets\"") != std::string::npos) {
                inWidgets = true;
            } else if (inWidgets && line.find("{") != std::string::npos) {
                currentWidget = Widget();
            } else if (inWidgets && line.find("}") != std::string::npos) {
                if (currentWidget.type != "") {
                    scene.widgets.push_back(currentWidget);
                }
            } else if (inWidgets && line.find("\"type\"") != std::string::npos) {
                currentWidget.type = extractStringValue(line);
            } else if (inWidgets && line.find("\"language\"") != std::string::npos) {
                currentWidget.properties["language"] = extractStringValue(line);
            } else if (inWidgets && line.find("\"row\"") != std::string::npos) {
                currentWidget.row = extractIntValue(line);
            } else if (inWidgets && line.find("\"col\"") != std::string::npos) {
                currentWidget.col = extractIntValue(line);
            } else if (inWidgets && line.find("\"width\"") != std::string::npos) {
                currentWidget.width = extractIntValue(line);
            } else if (inWidgets && line.find("\"height\"") != std::string::npos) {
                currentWidget.height = extractIntValue(line);
            } else if (inWidgets && line.find("\"margin\"") != std::string::npos) {
                currentWidget.margin = extractFloatValue(line);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] loadScene: Exception during file reading: " << e.what() << std::endl;
        fclose(file);
        return false;
    } catch (...) {
        std::cerr << "[ERROR] loadScene: Unknown exception during file reading" << std::endl;
        fclose(file);
        return false;
    }
    
    fclose(file);
    std::cout << "[DEBUG] loadScene: Parsed " << lineCount << " lines, widgets: " << scene.widgets.size() << std::endl;
    return true;
}
