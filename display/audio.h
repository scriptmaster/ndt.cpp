#ifndef AUDIO_H
#define AUDIO_H

#include <string>
#include <vector>

// Audio system for procedural bass sound generation
void initAudioGeneration(int seed);
void playBassSound(float duration, float fadeInDuration);
void updateAudio(float deltaTime);
void cleanupAudio();
int getAudioSeed();
void setAudioSeed(int seed);
bool saveAudioSeed(const std::string& filename);
bool loadAudioSeed(const std::string& filename);

// Waveform widget - amplitude values updated every 100ms
std::vector<float> getWaveformAmplitudes(); // Returns amplitude values for waveform bars

// Audio capture and STT
bool initAudioCapture(int sampleRate = 44100);
void cleanupAudioCapture();
void startAudioCapture();
void stopAudioCapture();
bool isAudioCapturing();
std::vector<short> getCapturedAudioSamples(); // Get captured audio samples for STT
std::string getAudioDeviceName(); // Get current audio device name

#endif // AUDIO_H
