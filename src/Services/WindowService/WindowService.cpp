#include "WindowService.h"
#include "WindowData.h"
#include "TextureLoader.h"
#include "WindowManager.h"
#include "AppLoop.h"
#include "../LoggingService/SceneLogger.h"
#include "../../App/AdminUtils.h"
#include <GLFW/glfw3.h>
#include <iostream>

/**
 * WindowService implementation
 * Manages GLFW windows and main loop
 */

WindowService::WindowService() : isAdmin_(false), stopped_(true) {
}

WindowService::~WindowService() {
}

void WindowService::Configure() {
    // No configuration needed
}

bool WindowService::Start() {
    std::cout << "[DEBUG] STEP 2: Checking admin status..." << std::endl;
    try {
        isAdmin_ = isRunningAsAdmin();
        if (isAdmin_) {
            std::cout << "[DEBUG] STEP 2: Running as administrator - SUCCESS" << std::endl;
        } else {
            std::cout << "[DEBUG] STEP 2: Not running as admin - SUCCESS" << std::endl;
        }
    } catch (...) {
        std::cerr << "[ERROR] STEP 2: Admin check failed" << std::endl;
        isAdmin_ = false;
    }

    std::cout << "[DEBUG] STEP 3: Creating windows..." << std::endl;
    try {
        windows_ = createWindows();
        std::cout << "[DEBUG] STEP 3: Windows created, count: " << windows_.size() << " - SUCCESS" << std::endl;
        
        if (windows_.empty()) {
            std::cerr << "[ERROR] STEP 3: No windows created" << std::endl;
            return false;
        }
        
        initSceneLogger();
        stopped_ = false;
        SetAdminStatus(isAdmin_);
        SetVSync(1); // Enable VSync by default
        return true;
    } catch (...) {
        std::cerr << "[ERROR] STEP 3: Window creation failed with exception" << std::endl;
        return false;
    }
}

void WindowService::Stop() {
    if (stopped_) {
        return;
    }
    std::cout << "[DEBUG] Stopping window service..." << std::endl;
    cleanupSceneLogger();
    cleanupWindows(windows_);
    stopped_ = true;
    std::cout << "[DEBUG] Window service stopped - SUCCESS" << std::endl;
}

std::vector<WindowData>& WindowService::GetWindows() {
    return windows_;
}

const std::vector<WindowData>& WindowService::GetWindows() const {
    return windows_;
}

int WindowService::RunLoop() {
    std::cout << "[DEBUG] STEP 9: Starting main loop..." << std::endl;
    try {
        runMainLoop(windows_);
        std::cout << "[DEBUG] STEP 9: Main loop exited - SUCCESS" << std::endl;
        return 0;
    } catch (...) {
        std::cerr << "[ERROR] STEP 9: Main loop failed with exception" << std::endl;
        return -1;
    }
}

void WindowService::SetAdminStatus(bool isAdmin) {
    isAdmin_ = isAdmin;
    std::cout << "[DEBUG] STEP 4: Setting admin status for windows..." << std::endl;
    try {
        for (auto& wd : windows_) {
            wd.isAdmin = isAdmin;
        }
        std::cout << "[DEBUG] STEP 4: Admin status set - SUCCESS" << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] STEP 4: Setting admin status failed" << std::endl;
    }
}

void WindowService::SetVSync(int interval) {
    std::cout << "[DEBUG] STEP 6: Setting VSync..." << std::endl;
    try {
        glfwSwapInterval(interval);
        std::cout << "[DEBUG] STEP 6: VSync set - SUCCESS" << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] STEP 6: VSync setting failed" << std::endl;
    }
}
