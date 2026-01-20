#ifndef SPEECH_GATE_H
#define SPEECH_GATE_H

#include <functional>
#include <chrono>

/**
 * SpeechGate - Detects speech vs silence state transitions
 * 
 * Single responsibility: Speech/silence detection with hysteresis
 * 
 * Uses RMS values and noise floor to determine when speech starts and ends.
 * Implements hysteresis to prevent flickering between states.
 */
class SpeechGate {
public:
    /**
     * Callback function type for speech events
     */
    using SpeechEventCallback = std::function<void()>;
    
    /**
     * Constructor
     * @param speechThresholdMultiplier Multiplier for noise floor to determine speech threshold (default: 2.5)
     * @param silenceThresholdMultiplier Multiplier for noise floor to determine silence threshold (default: 1.5)
     * @param speechStartHoldMs Minimum time above threshold to start speech (default: 200ms)
     * @param speechEndHoldMs Minimum time below threshold to end speech (default: 500ms)
     */
    SpeechGate(float speechThresholdMultiplier = 2.5f,
               float silenceThresholdMultiplier = 1.5f,
               double speechStartHoldMs = 200.0,
               double speechEndHoldMs = 500.0);
    
    /**
     * Update gate state based on current RMS and noise floor
     * @param rms Current RMS value
     * @param noiseFloor Calibrated noise floor value
     * @return true if speech state changed
     */
    bool update(float rms, float noiseFloor);
    
    /**
     * Check if currently speaking
     * @return true if in SPEECH state
     */
    bool isSpeaking() const { return isSpeaking_; }
    
    /**
     * Set callback for speech start events
     */
    void setOnSpeechStart(SpeechEventCallback callback) {
        onSpeechStart_ = callback;
    }
    
    /**
     * Set callback for speech end events
     */
    void setOnSpeechEnd(SpeechEventCallback callback) {
        onSpeechEnd_ = callback;
    }
    
    /**
     * Reset gate to silence state
     */
    void reset();
    
    /**
     * Get current speech threshold (noiseFloor * multiplier)
     */
    float getSpeechThreshold(float noiseFloor) const {
        return noiseFloor * speechThresholdMultiplier_;
    }
    
    /**
     * Get current silence threshold (noiseFloor * multiplier)
     */
    float getSilenceThreshold(float noiseFloor) const {
        return noiseFloor * silenceThresholdMultiplier_;
    }
    
private:
    enum class State {
        SILENCE,
        SPEECH
    };
    
    float speechThresholdMultiplier_;
    float silenceThresholdMultiplier_;
    double speechStartHoldMs_;
    double speechEndHoldMs_;
    
    State state_;
    bool isSpeaking_;
    double speechTimeAccumulatorMs_;   // Accumulates time above speech threshold
    double silenceTimeAccumulatorMs_;  // Accumulates time below silence threshold
    std::chrono::steady_clock::time_point lastUpdateTime_;
    
    SpeechEventCallback onSpeechStart_;
    SpeechEventCallback onSpeechEnd_;
    
    void transitionToSpeech();
    void transitionToSilence();
};

#endif // SPEECH_GATE_H
