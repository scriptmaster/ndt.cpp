#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <string>
#include <vector>

/**
 * AudioCapture - Audio capture functions
 * 
 * Single responsibility: Windows audio capture for microphone input
 */

// Audio capture lifecycle
bool initAudioCapture(int sampleRate = 44100);
void cleanupAudioCapture();
void startAudioCapture();
void stopAudioCapture();
bool isAudioCapturing();
std::vector<short> getCapturedAudioSamples();

// List all available audio devices (logs all devices, not just default)
void listAllAudioDevices();

#endif // AUDIO_CAPTURE_H
