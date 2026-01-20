#include "AudioService.h"
#include "AudioSystem.h"
#include "AudioCapture.h"
#include "../NetworkService/NetworkSystem.h"
#include "../LoggingService/SceneLogger.h"
#include <iostream>

/**
 * AudioService implementation
 * Wraps audio initialization and cleanup
 */

AudioService::AudioService() : initialized_(false) {
}

AudioService::~AudioService() {
}

void AudioService::Configure() {
    // No configuration needed
}

bool AudioService::Start() {
    std::cout << "[DEBUG] STEP 8: Initializing audio system..." << std::endl;
    try {
        initialized_ = initializeSystems();
        if (!initialized_) {
            std::cerr << "[WARNING] STEP 8: Audio initialization failed, continuing without audio" << std::endl;
        } else {
            std::cout << "[DEBUG] STEP 8: Audio initialized - SUCCESS" << std::endl;
        }
        // Always return true - audio is non-fatal
        return true;
    } catch (...) {
        std::cerr << "[ERROR] STEP 8: Audio initialization failed with exception" << std::endl;
        // Continue anyway - audio is non-fatal
        return true;
    }
}

void AudioService::Stop() {
    if (initialized_) {
        // Stop and cleanup audio capture
        try {
            stopAudioCapture();
            cleanupAudioCapture();
        } catch (...) {
            // Continue cleanup even if audio capture cleanup fails
        }
        
        // Cleanup audio system
        try {
            cleanupAudio();
        } catch (...) {
            // Continue cleanup even if audio cleanup fails
        }
        
        // Cleanup network subsystem (initialized in initializeSystems)
        try {
            cleanupNetwork();
        } catch (...) {
            // Continue cleanup even if network cleanup fails
        }
        
        // Cleanup scene and audio loggers (initialized in initializeSystems)
        try {
            cleanupSceneLogger();
            cleanupAudioLogger();
        } catch (...) {
            // Continue cleanup even if logger cleanup fails
        }
    }
}
