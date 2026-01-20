#ifndef AUDIO_WAVEFORM_H
#define AUDIO_WAVEFORM_H

#include <vector>

/**
 * AudioWaveform - Waveform processing functions
 * 
 * Single responsibility: RMS-based waveform calculation and bar history
 */

// Configure waveform update rate (in FPS)
// Default: 10fps (updates every 100ms at 60fps main loop = every 6 frames)
void setWaveformUpdateFPS(int fps);
int getWaveformUpdateFPS();

// Internal waveform processing (called from AudioCapture)
void updateAudioSamples(const float* samples, int numSamples);
float calculateRMS();

#endif // AUDIO_WAVEFORM_H
