#include "AudioSystem.h"
#include "../LoggingService/SceneLogger.h"
#include "AudioCapture.h"
#include "AudioWaveform.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <cstring>

/**
 * AudioSystem - Audio system initialization and waveform processing
 * 
 * Single responsibility: Audio generation, seed management, waveform processing
 */

static int audioSeed = 12345;
static bool audioInitialized = false;

// Forward declarations for audio capture
extern bool initAudioCapture(int sampleRate);
extern void startAudioCapture();

// Forward declarations for network (implemented in NetworkSystem.cpp)
extern bool initNetwork();
extern void cleanupNetwork();

bool initializeSystems() {
    std::cout << "[DEBUG] Initializing audio..." << std::endl;
    
    initSceneLogger();
    
    try {
        int seed = 12345;
        if (!loadAudioSeed("config/audio_seed.txt")) {
            std::cout << "[DEBUG] Using default audio seed: " << seed << std::endl;
        } else {
            seed = getAudioSeed();
            std::cout << "[DEBUG] Loaded audio seed from config: " << seed << std::endl;
        }
        
        if (!initNetwork()) {
            std::cerr << "[WARNING] Network initialization failed - STT will not work" << std::endl;
        } else {
            std::cout << "[DEBUG] Network initialized successfully" << std::endl;
        }
        
        initAudioGeneration(seed);
        std::cout << "[DEBUG] Audio generation initialized successfully" << std::endl;
        
        // Configure waveform update FPS (default: 10fps, configurable via WAVEFORM_FPS env var)
        int waveformFPS = 10;
        const char* envFPS = std::getenv("WAVEFORM_FPS");
        if (envFPS) {
            int envFPSValue = std::atoi(envFPS);
            if (envFPSValue >= 1 && envFPSValue <= 60) {
                waveformFPS = envFPSValue;
            }
        }
        setWaveformUpdateFPS(waveformFPS);
        std::cout << "[DEBUG] Waveform update rate set to " << waveformFPS << "fps" << std::endl;
        
        if (!initAudioCapture(44100)) {
            std::cerr << "[WARNING] Audio capture initialization failed - STT will not receive audio" << std::endl;
        } else {
            std::cout << "[DEBUG] Audio capture initialized successfully" << std::endl;
            startAudioCapture();
            std::cout << "[DEBUG] Audio capture started" << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during audio initialization: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during audio initialization" << std::endl;
        return false;
    }
}

void initAudioGeneration(int seed) {
    audioSeed = seed;
    audioInitialized = true;
    srand(seed);
}

void cleanupAudio() {
    audioInitialized = false;
}

// Note: updateAudio(), getWaveformAmplitudes(), and getAudioDeviceName() are implemented in:
// - AudioWaveform.cpp: updateAudio(), getWaveformAmplitudes()
// - AudioCapture.cpp: getAudioDeviceName()
// They are declared in AudioSystem.h for convenience but implemented in their respective files

int getAudioSeed() {
    return audioSeed;
}

void setAudioSeed(int seed) {
    audioSeed = seed;
    if (audioInitialized) {
        srand(seed);
    }
}

bool saveAudioSeed(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "w");
    if (!file) {
        return false;
    }
    
    int result = fprintf(file, "%d\n", audioSeed);
    fclose(file);
    
    return (result > 0);
}

bool loadAudioSeed(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        return false;
    }
    
    int result = fscanf(file, "%d", &audioSeed);
    fclose(file);
    
    if (result != 1) {
        audioSeed = 12345;
        return false;
    }
    
    if (audioInitialized) {
        srand(audioSeed);
    }
    return true;
}

// Note: getWaveformAmplitudes() and getAudioDeviceName() are implemented 
// in AudioWaveform.cpp and AudioCapture.cpp respectively
// They are declared in AudioSystem.h for convenience
