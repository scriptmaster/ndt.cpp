#include "LocalConfigService.h"
#include <fstream>
#include <iostream>
#include <sstream>

/**
 * LocalConfigService implementation
 * Manages configuration files with default values
 */

LocalConfigService::LocalConfigService() : audioSeed_(DEFAULT_AUDIO_SEED) {
}

LocalConfigService::~LocalConfigService() {
}

void LocalConfigService::Configure() {
    // No configuration needed
}

bool LocalConfigService::Start() {
    // Service is ready to use
    return true;
}

void LocalConfigService::Stop() {
    // No cleanup needed
}

int LocalConfigService::GetAudioSeed() const {
    return audioSeed_;
}

void LocalConfigService::SetAudioSeed(int seed) {
    audioSeed_ = seed;
}

bool LocalConfigService::LoadAudioSeed(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        if (iss >> audioSeed_) {
            return true;
        }
    }

    return false;
}

bool LocalConfigService::SaveAudioSeed(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << audioSeed_;
    return true;
}
