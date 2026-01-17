#ifndef ADMIN_H
#define ADMIN_H

#include <GLFW/glfw3.h>
#include <string>
#include "scene.h"

struct WindowData;

// Admin mode functions
bool isRunningAsAdmin();
bool checkTetraClick(WindowData& wd, double xpos, double ypos, int windowWidth, int windowHeight, double currentTime);
void renderAdminModeText(int windowWidth, int windowHeight);
void renderTetraClickIndicator(int windowWidth, int windowHeight, int clickCount);
bool loadAdminScene(const std::string& sceneFile, Scene& scene);
void handleAdminClick(WindowData& wd, double xpos, double ypos, int windowWidth, int windowHeight, double currentTime);

#endif // ADMIN_H
