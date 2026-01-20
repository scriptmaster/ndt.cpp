#ifndef RMS_ANALYZER_H
#define RMS_ANALYZER_H

#include <deque>
#include <mutex>
#include <cstdint>

/**
 * RMSAnalyzer - Computes RMS (Root Mean Square) over a fixed-time sliding window
 * 
 * Single responsibility: RMS calculation only
 * 
 * Thread-safe implementation that maintains a fixed-time window of audio samples
 * (e.g., last 100ms) and computes RMS over that window.
 */
class RMSAnalyzer {
public:
    /**
     * Constructor
     * @param sampleRate Sample rate in Hz (e.g., 44100)
     * @param windowDurationMs Window duration in milliseconds (e.g., 100ms)
     */
    RMSAnalyzer(int sampleRate, double windowDurationMs = 100.0);
    
    /**
     * Update analyzer with new PCM samples
     * @param samples Pointer to int16 PCM samples
     * @param numSamples Number of samples
     * @return Current RMS value (0.0 if window not yet full)
     */
    float update(const int16_t* samples, int numSamples);
    
    /**
     * Get current RMS value
     * @return RMS value (0.0 if window not yet full)
     */
    float getRMS() const;
    
    /**
     * Reset the window (clears all samples)
     */
    void reset();
    
    /**
     * Check if window is full and RMS is valid
     * @return true if window has enough samples for valid RMS
     */
    bool isWindowFull() const;
    
    /**
     * Get window duration in milliseconds
     */
    double getWindowDurationMs() const { return windowDurationMs_; }
    
    /**
     * Get window size in samples
     */
    int getWindowSize() const { return windowSize_; }
    
private:
    int sampleRate_;
    double windowDurationMs_;
    int windowSize_;
    std::deque<float> window_;  // Fixed-size sliding window (normalized float samples)
    mutable std::mutex mutex_;
    
    float computeRMS() const;
};

#endif // RMS_ANALYZER_H
