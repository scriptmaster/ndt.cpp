#ifndef AUDIO_CAPTURE_ORCHESTRATOR_H
#define AUDIO_CAPTURE_ORCHESTRATOR_H

#include "RMSAnalyzer.h"
#include "NoiseCalibrator.h"
#include "SpeechGate.h"
#include "AudioSegmentBuffer.h"
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>

/**
 * AudioCaptureOrchestrator - Coordinates audio capture pipeline
 * 
 * Single responsibility: Orchestrate audio processing pipeline
 * 
 * Receives PCM samples from platform-specific capture, feeds them through
 * the analysis pipeline (RMS -> Noise Calibration -> Speech Detection -> Buffering),
 * and emits debug information.
 * 
 * Does NOT depend on UI or STT logic - clean separation of concerns.
 */
class AudioCaptureOrchestrator {
public:
    /**
     * Callback for when a speech segment is ready
     * Parameter: PCM samples ready for Whisper
     */
    using SegmentReadyCallback = std::function<void(const std::vector<int16_t>&)>;
    
    /**
     * Callback for debug information
     */
    using DebugInfoCallback = std::function<void(float rms, float noiseFloor, bool isSpeaking, size_t bufferSize)>;
    
    /**
     * Constructor
     * @param sampleRate Sample rate in Hz (e.g., 44100)
     */
    explicit AudioCaptureOrchestrator(int sampleRate = 44100);
    
    /**
     * Process new PCM samples from capture
     * @param samples Pointer to int16 PCM samples
     * @param numSamples Number of samples
     */
    void processSamples(const int16_t* samples, int numSamples);
    
    /**
     * Start audio processing (call when capture starts)
     */
    void start();
    
    /**
     * Stop audio processing (call when capture stops)
     */
    void stop();
    
    /**
     * Set callback for when speech segment is ready
     */
    void setOnSegmentReady(SegmentReadyCallback callback) {
        onSegmentReady_ = callback;
    }
    
    /**
     * Set callback for debug information
     */
    void setOnDebugInfo(DebugInfoCallback callback) {
        onDebugInfo_ = callback;
    }
    
    /**
     * Get current RMS value
     */
    float getCurrentRMS() const {
        return rmsAnalyzer_.getRMS();
    }
    
    /**
     * Get noise floor value
     */
    float getNoiseFloor() const {
        return noiseCalibrator_.getNoiseFloor();
    }
    
    /**
     * Check if currently speaking
     */
    bool isSpeaking() const {
        return speechGate_.isSpeaking();
    }
    
    /**
     * Check if noise calibration is complete
     */
    bool isCalibrated() const {
        return noiseCalibrator_.isCalibrated();
    }
    
    /**
     * Get current buffer size
     */
    size_t getBufferSize() const;
    
private:
    int sampleRate_;
    
    RMSAnalyzer rmsAnalyzer_;
    NoiseCalibrator noiseCalibrator_;
    SpeechGate speechGate_;
    AudioSegmentBuffer segmentBuffer_;
    
    bool processing_;
    SegmentReadyCallback onSegmentReady_;
    DebugInfoCallback onDebugInfo_;
    
    void handleSpeechStart();
    void handleSpeechEnd();
    void emitDebugInfo();
};

#endif // AUDIO_CAPTURE_ORCHESTRATOR_H
