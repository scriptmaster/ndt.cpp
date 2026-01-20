#include "RMSAnalyzer.h"
#include <cmath>
#include <algorithm>

RMSAnalyzer::RMSAnalyzer(int sampleRate, double windowDurationMs)
    : sampleRate_(sampleRate), windowDurationMs_(windowDurationMs) {
    // Calculate window size in samples
    windowSize_ = static_cast<int>(sampleRate * windowDurationMs / 1000.0);
    if (windowSize_ < 1) {
        windowSize_ = 1;  // Minimum 1 sample
    }
    // Note: std::deque doesn't have reserve(), but it's efficient for push_back/pop_front
}

float RMSAnalyzer::update(const int16_t* samples, int numSamples) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Add samples to window (convert int16 to normalized float)
    for (int i = 0; i < numSamples; i++) {
        float normalized = static_cast<float>(samples[i]) / 32768.0f;
        window_.push_back(normalized);
        
        // Keep window size fixed - remove oldest when full
        if (window_.size() > static_cast<size_t>(windowSize_)) {
            window_.pop_front();
        }
    }
    
    return computeRMS();
}

float RMSAnalyzer::getRMS() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return computeRMS();
}

void RMSAnalyzer::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    window_.clear();
}

bool RMSAnalyzer::isWindowFull() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return window_.size() >= static_cast<size_t>(windowSize_);
}

float RMSAnalyzer::computeRMS() const {
    // RMS is only valid if window is full
    if (window_.size() < static_cast<size_t>(windowSize_)) {
        return 0.0f;
    }
    
    // Compute RMS: sqrt(sum(samples^2) / count)
    float sumSquared = 0.0f;
    for (float sample : window_) {
        sumSquared += sample * sample;
    }
    
    return std::sqrt(sumSquared / window_.size());
}
