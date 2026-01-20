#include "AudioCaptureOrchestrator.h"
#include <iostream>

AudioCaptureOrchestrator::AudioCaptureOrchestrator(int sampleRate)
    : sampleRate_(sampleRate),
      rmsAnalyzer_(sampleRate, 100.0),  // 100ms RMS window
      noiseCalibrator_(300.0),           // 300ms calibration window
      speechGate_(2.5f, 1.5f, 200.0, 500.0),  // Speech: 2.5x, Silence: 1.5x, Start: 200ms, End: 500ms
      segmentBuffer_(sampleRate, 100.0, 200.0),  // 100ms pre-padding, 200ms post-padding
      processing_(false) {
    
    // Set up speech gate callbacks
    speechGate_.setOnSpeechStart([this]() { handleSpeechStart(); });
    speechGate_.setOnSpeechEnd([this]() { handleSpeechEnd(); });
}

void AudioCaptureOrchestrator::start() {
    processing_ = true;
    noiseCalibrator_.startCalibration();
    rmsAnalyzer_.reset();
    speechGate_.reset();
    segmentBuffer_.clear();

    // Debug: verify initialization
    std::cout << "[DEBUG] Orchestrator started - Buffer size: " << segmentBuffer_.size() << std::endl;
}

void AudioCaptureOrchestrator::stop() {
    processing_ = false;
    
    // Finalize any pending segment
    if (speechGate_.isSpeaking()) {
        segmentBuffer_.finalizeSegment();
        if (segmentBuffer_.hasSegment()) {
            auto segment = segmentBuffer_.consumeSegment();
            if (onSegmentReady_ && !segment.empty()) {
                onSegmentReady_(segment);
            }
        }
    }
}

void AudioCaptureOrchestrator::processSamples(const int16_t* samples, int numSamples) {
    if (!processing_ || numSamples <= 0 || samples == nullptr) {
        return;
    }
    
    // Step 1: Update RMS analyzer
    float rms = rmsAnalyzer_.update(samples, numSamples);
    
    // Step 2: Update noise calibrator (only during calibration window)
    if (!noiseCalibrator_.isCalibrated()) {
        noiseCalibrator_.addRMSValue(rms);
    }
    
    // Step 3: Update speech gate (only after calibration)
    bool speechStateChanged = false;
    if (noiseCalibrator_.isCalibrated()) {
        float noiseFloor = noiseCalibrator_.getNoiseFloor();
        speechStateChanged = speechGate_.update(rms, noiseFloor);
    }
    
    // Step 4: Update segment buffer
    segmentBuffer_.addSamples(samples, numSamples, speechGate_.isSpeaking());
    
    // Step 5: Emit debug information
    emitDebugInfo();
}

void AudioCaptureOrchestrator::handleSpeechStart() {
    // Speech started - segment buffer will start collecting
    // No action needed here, but hook is available for future use
}

void AudioCaptureOrchestrator::handleSpeechEnd() {
    // Speech ended - finalize segment and notify
    segmentBuffer_.finalizeSegment();
    
    if (segmentBuffer_.hasSegment()) {
        auto segment = segmentBuffer_.consumeSegment();
        
        if (!segment.empty()) {
            // TODO: Whisper integration point
            // Send segment to Whisper STT service here
            // Example: whisperService->transcribe(segment, sampleRate_);
            
            if (onSegmentReady_) {
                onSegmentReady_(segment);
            }
        }
    }
}

void AudioCaptureOrchestrator::emitDebugInfo() {
    if (onDebugInfo_) {
        float rms = rmsAnalyzer_.getRMS();
        float noiseFloor = noiseCalibrator_.getNoiseFloor();
        bool isSpeaking = speechGate_.isSpeaking();
        size_t bufferSize = segmentBuffer_.size();

        // Debug: print buffer size to stderr to avoid mixing with test output
        static int debugCount = 0;
        if (debugCount++ % 10 == 0) {  // Every 10th call
            std::cerr << "[DEBUG] RMS: " << rms << " Noise: " << noiseFloor
                      << " Speaking: " << isSpeaking << " Buffer: " << bufferSize << std::endl;
        }

        onDebugInfo_(rms, noiseFloor, isSpeaking, bufferSize);
    }
}
