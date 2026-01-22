#include "TTSService.h"
#include "../Common/SidecarManager.h"

/**
 * TTSService implementation
 * Stub for future TTS functionality
 */

TTSService::TTSService() {
}

TTSService::~TTSService() {
}

void TTSService::Configure() {
    // Stub - no configuration yet
}

bool TTSService::Start() {
    if (!SidecarManager::EnsureLlamaServerRunning()) {
        std::cerr << "[ERROR] TTSService: Failed to ensure llama-server is running" << std::endl;
        return false;
    }
    return true;
}

void TTSService::Stop() {
    // Stub - no cleanup needed
}
