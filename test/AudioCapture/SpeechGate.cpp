#include "SpeechGate.h"
#include <iostream>
#include <algorithm>

SpeechGate::SpeechGate(float speechThresholdMultiplier,
                     float silenceThresholdMultiplier,
                     double speechStartHoldMs,
                     double speechEndHoldMs)
    : speechThresholdMultiplier_(speechThresholdMultiplier),
      silenceThresholdMultiplier_(silenceThresholdMultiplier),
      speechStartHoldMs_(speechStartHoldMs),
      speechEndHoldMs_(speechEndHoldMs),
      state_(State::SILENCE),
      isSpeaking_(false),
      speechTimeAccumulatorMs_(0.0),
      silenceTimeAccumulatorMs_(0.0),
      lastUpdateTime_() {
}

bool SpeechGate::update(float rms, float noiseFloor) {
    if (noiseFloor <= 0.0f) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    double deltaTimeMs = 0.0;

    if (lastUpdateTime_ != std::chrono::steady_clock::time_point{}) {
        deltaTimeMs = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
            now - lastUpdateTime_).count();
        // Cap delta time to prevent issues with very long pauses
        if (deltaTimeMs > 1000.0) {  // Max 1 second delta
            deltaTimeMs = 1000.0;
        }
    }
    lastUpdateTime_ = now;

    // Compute thresholds relative to noise floor, with minimum bounds
    float speechThreshold = std::max(noiseFloor * speechThresholdMultiplier_, 0.0001f);
    float silenceThreshold = std::max(noiseFloor * silenceThresholdMultiplier_, 0.00005f);
    
    bool stateChanged = false;
    
    if (state_ == State::SILENCE) {
        // Accumulate speech time only if above threshold
        if (rms >= speechThreshold) {
            speechTimeAccumulatorMs_ += deltaTimeMs;
            if (speechTimeAccumulatorMs_ >= speechStartHoldMs_) {
                transitionToSpeech();
                stateChanged = true;
            }
        } else if (rms < silenceThreshold) {
            // Reset accumulator only if it falls below the lower threshold (hysteresis)
            speechTimeAccumulatorMs_ = 0.0;
        }
        // If between thresholds, we hold the current accumulator value (stable)
    } else {
        // Currently SPEECH - check for transition to silence
        if (rms < silenceThreshold) {
            silenceTimeAccumulatorMs_ += deltaTimeMs;
            if (silenceTimeAccumulatorMs_ >= speechEndHoldMs_) {
                transitionToSilence();
                stateChanged = true;
            }
        } else if (rms >= speechThreshold) {
            // Reset accumulator only if it rises above speech threshold
            silenceTimeAccumulatorMs_ = 0.0;
        }
        // If between thresholds, we hold the current accumulator value (stable)
    }
    
    return stateChanged;
}

void SpeechGate::transitionToSpeech() {
    state_ = State::SPEECH;
    isSpeaking_ = true;
    speechTimeAccumulatorMs_ = 0.0;
    silenceTimeAccumulatorMs_ = 0.0;
    
    if (onSpeechStart_) {
        onSpeechStart_();
    }
}

void SpeechGate::transitionToSilence() {
    state_ = State::SILENCE;
    isSpeaking_ = false;
    speechTimeAccumulatorMs_ = 0.0;
    silenceTimeAccumulatorMs_ = 0.0;
    
    if (onSpeechEnd_) {
        onSpeechEnd_();
    }
}

void SpeechGate::reset() {
    state_ = State::SILENCE;
    isSpeaking_ = false;
    speechTimeAccumulatorMs_ = 0.0;
    silenceTimeAccumulatorMs_ = 0.0;
    lastUpdateTime_ = std::chrono::steady_clock::time_point{};  // Reset to invalid time
}
