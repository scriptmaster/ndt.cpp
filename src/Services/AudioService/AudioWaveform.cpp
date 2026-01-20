#include "AudioWaveform.h"
#include <cmath>
#include <vector>
#include <algorithm>

/**
 * AudioWaveform - Waveform processing implementation
 * 
 * Single responsibility: RMS-based waveform calculation
 */

// Waveform widget state
static const int SAMPLE_BUFFER_SIZE = 512;
static const int RMS_HISTORY_SIZE = 30;
static const int MAX_BARS = 300;
static const float CLAMP_THRESHOLD = 0.02f;
static const float SILENCE_THRESHOLD = 0.001f;

// Sample buffer (circular buffer)
static std::vector<float> sampleBuffer(SAMPLE_BUFFER_SIZE, 0.0f);
static int sampleBufferWriteIndex = 0;
static int sampleBufferCount = 0;

// RMS history
static std::vector<float> rmsHistory;
static float maxRMSSeen = 0.0001f;

// Bar data structure
struct BarData {
    float height;
};

// Bar history
static std::vector<BarData> barHistory;

// Update tracking - configurable FPS
static int frameCount = 0;
static int waveformUpdateFPS = 10; // Default: 10fps
static int updateIntervalFrames = 6; // 60fps / 10fps = 6 frames

void setWaveformUpdateFPS(int fps) {
    if (fps < 1) fps = 1;
    if (fps > 60) fps = 60;
    waveformUpdateFPS = fps;
    // Calculate frame interval: assume main loop runs at 60fps
    updateIntervalFrames = 60 / waveformUpdateFPS;
    if (updateIntervalFrames < 1) updateIntervalFrames = 1;
}

int getWaveformUpdateFPS() {
    return waveformUpdateFPS;
}

void updateAudioSamples(const float* samples, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        sampleBuffer[sampleBufferWriteIndex] = samples[i];
        sampleBufferWriteIndex = (sampleBufferWriteIndex + 1) % SAMPLE_BUFFER_SIZE;
        if (sampleBufferCount < SAMPLE_BUFFER_SIZE) {
            sampleBufferCount++;
        }
    }
}

float calculateRMS() {
    if (sampleBufferCount == 0) return 0.0f;
    
    float sumSquared = 0.0f;
    for (int i = 0; i < sampleBufferCount; i++) {
        float sample = sampleBuffer[i];
        sumSquared += sample * sample;
    }
    
    return sqrtf(sumSquared / sampleBufferCount);
}

// Add a new bar to history
static void addBar(float heightPercent) {
    BarData bar;
    bar.height = heightPercent;
    
    barHistory.insert(barHistory.begin(), bar);
    
    if (barHistory.size() > MAX_BARS) {
        barHistory.resize(MAX_BARS);
    }
}

std::vector<float> getWaveformAmplitudes() {
    std::vector<float> heights;
    heights.reserve(barHistory.size());
    for (const auto& bar : barHistory) {
        heights.push_back(bar.height);
    }
    return heights;
}

void updateAudio(float deltaTime) {
    frameCount++;
    
    if (frameCount % updateIntervalFrames != 0) {
        return;
    }
    
#ifdef _WIN32
    extern bool isAudioCapturing();
    if (!isAudioCapturing()) {
        return;
    }
#endif
    
    float rms = calculateRMS();
    
    if (rms < SILENCE_THRESHOLD) {
        rms = 0.0f;
    }
    
    rmsHistory.insert(rmsHistory.begin(), rms);
    if (rmsHistory.size() > RMS_HISTORY_SIZE) {
        rmsHistory.resize(RMS_HISTORY_SIZE);
    }
    
    maxRMSSeen = 0.0001f;
    for (float h : rmsHistory) {
        if (h > maxRMSSeen) {
            maxRMSSeen = h;
        }
    }
    
    float heightPercent = (maxRMSSeen > 0.0001f) ? (rms / maxRMSSeen) : 0.0f;
    
    if (heightPercent < CLAMP_THRESHOLD) {
        heightPercent = 0.0f;
    }
    
    float barHeight = fminf(heightPercent * 1.6f, 1.0f);
    
    addBar(barHeight);
}
