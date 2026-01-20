#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include <string>
#include <vector>

/**
 * AudioSystem - Audio system initialization and waveform functions
 * 
 * Single responsibility: Audio generation, waveform processing, seed management
 */

// Audio system initialization
bool initializeSystems();
void initAudioGeneration(int seed);
void cleanupAudio();
void updateAudio(float deltaTime);

// Waveform update rate configuration (called from initializeSystems)
// This is a forward declaration - actual implementation in AudioWaveform.cpp
void setWaveformUpdateFPS(int fps);
int getWaveformUpdateFPS();

// Audio seed management
int getAudioSeed();
void setAudioSeed(int seed);
bool loadAudioSeed(const std::string& filename);
bool saveAudioSeed(const std::string& filename);

// Waveform functions
std::vector<float> getWaveformAmplitudes();
std::string getAudioDeviceName();

#endif // AUDIO_SYSTEM_H
