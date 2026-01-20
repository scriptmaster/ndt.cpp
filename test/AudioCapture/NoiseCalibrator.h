#ifndef NOISE_CALIBRATOR_H
#define NOISE_CALIBRATOR_H

#include <vector>
#include <mutex>
#include <chrono>

/**
 * NoiseCalibrator - Estimates background noise floor level
 * 
 * Single responsibility: Noise floor calibration
 * 
 * Collects RMS values during a calibration window (first 300ms after start)
 * and computes the average to establish a baseline noise floor.
 * Once calibrated, the noise floor value is locked and used for speech detection.
 */
class NoiseCalibrator {
public:
    /**
     * Constructor
     * @param calibrationDurationMs Duration of calibration window in milliseconds (default: 300ms)
     */
    explicit NoiseCalibrator(double calibrationDurationMs = 300.0);
    
    /**
     * Start calibration (call when audio capture starts)
     */
    void startCalibration();
    
    /**
     * Add RMS value for calibration
     * @param rms Current RMS value
     * @return true if calibration is complete
     */
    bool addRMSValue(float rms);
    
    /**
     * Check if calibration is complete
     * @return true if noise floor has been calculated and locked
     */
    bool isCalibrated() const;
    
    /**
     * Get the calibrated noise floor value
     * @return Noise floor RMS value (0.0 if not yet calibrated)
     */
    float getNoiseFloor() const;
    
    /**
     * Reset calibrator (for re-calibration)
     */
    void reset();
    
    /**
     * Get calibration duration in milliseconds
     */
    double getCalibrationDurationMs() const { return calibrationDurationMs_; }
    
private:
    double calibrationDurationMs_;
    std::vector<float> rmsSamples_;
    float noiseFloor_;
    bool calibrated_;
    std::chrono::steady_clock::time_point calibrationStartTime_;
    mutable std::mutex mutex_;
    
    void computeNoiseFloor();
};

#endif // NOISE_CALIBRATOR_H
