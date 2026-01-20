#ifndef AUDIO_SEGMENT_BUFFER_H
#define AUDIO_SEGMENT_BUFFER_H

#include <vector>
#include <cstdint>

/**
 * AudioSegmentBuffer - Collects PCM samples during speech segments
 * 
 * Single responsibility: Buffering PCM data for future STT processing
 * 
 * Collects raw PCM samples (int16) only while speech is detected.
 * Adds padding before and after speech if possible.
 * Provides contiguous buffers ready for Whisper integration.
 */
class AudioSegmentBuffer {
public:
    /**
     * Constructor
     * @param sampleRate Sample rate in Hz (for padding calculations)
     * @param prePaddingMs Padding to add before speech start (default: 100ms)
     * @param postPaddingMs Padding to add after speech end (default: 200ms)
     */
    AudioSegmentBuffer(int sampleRate, 
                      double prePaddingMs = 100.0,
                      double postPaddingMs = 200.0);
    
    /**
     * Add samples to buffer (only if currently speaking)
     * @param samples Pointer to int16 PCM samples
     * @param numSamples Number of samples
     * @param isSpeaking Current speech state
     */
    void addSamples(const int16_t* samples, int numSamples, bool isSpeaking);
    
    /**
     * Mark speech as ended and prepare segment for consumption
     * Call this when speech ends to finalize the segment
     */
    void finalizeSegment();
    
    /**
     * Check if a complete segment is available
     * @return true if segment is ready for consumption
     */
    bool hasSegment() const { return segmentReady_; }
    
    /**
     * Consume the buffered segment
     * @return Contiguous PCM buffer ready for Whisper (empty if no segment)
     */
    std::vector<int16_t> consumeSegment();
    
    /**
     * Clear buffer (discard current segment)
     */
    void clear();
    
    /**
     * Get current buffer size in samples
     */
    size_t size() const;
    
    /**
     * Get total samples collected (cumulative across all segments)
     */
    size_t getTotalSamplesCollected() const { return totalSamplesCollected_; }
    
private:
    int sampleRate_;
    double prePaddingMs_;
    double postPaddingMs_;
    
    std::vector<int16_t> buffer_;
    std::vector<int16_t> prePaddingBuffer_;  // Samples before speech started
    bool segmentReady_;
    bool wasSpeaking_;
    size_t totalSamplesCollected_;
    
    int samplesForDuration(double durationMs) const {
        return static_cast<int>(sampleRate_ * durationMs / 1000.0);
    }
    
    void addPrePadding();
    void addPostPadding();
};

#endif // AUDIO_SEGMENT_BUFFER_H
