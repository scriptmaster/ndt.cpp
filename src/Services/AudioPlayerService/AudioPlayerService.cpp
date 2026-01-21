#include "AudioPlayerService.h"
#include "AudioSeed.h"
#include "AudioGeneration.h"
#include "../AudioCaptureService/AudioWaveform.h"
#include <cstdlib>
#include <iostream>

AudioPlayerService::AudioPlayerService() : initialized_(false) {
}

AudioPlayerService::~AudioPlayerService() {
}

void AudioPlayerService::Configure() {
    // No configuration needed
}

bool AudioPlayerService::Start() {
    std::cout << "[DEBUG] Initializing audio player..." << std::endl;
    try {
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

        int seed = 12345;
        if (loadAudioSeed("config/audio_seed.txt")) {
            seed = getAudioSeed();
            std::cout << "[DEBUG] Loaded audio seed from config: " << seed << std::endl;
        } else {
            setAudioSeed(seed);
            std::cout << "[DEBUG] Using default audio seed: " << seed << std::endl;
        }

        initAudioGeneration(seed);
        initialized_ = true;
        std::cout << "[DEBUG] Audio player initialized - SUCCESS" << std::endl;
        return true;
    } catch (...) {
        std::cerr << "[ERROR] Audio player initialization failed" << std::endl;
        // Non-fatal
        return true;
    }
}

void AudioPlayerService::Stop() {
    if (!initialized_) {
        return;
    }
    std::cout << "[DEBUG] Stopping audio player..." << std::endl;
    cleanupAudio();
    initialized_ = false;
    std::cout << "[DEBUG] Audio player stopped - SUCCESS" << std::endl;
}
