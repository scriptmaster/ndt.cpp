#include "NoiseCalibrator.h"
#include <algorithm>
#include <numeric>

NoiseCalibrator::NoiseCalibrator(double calibrationDurationMs)
    : calibrationDurationMs_(calibrationDurationMs), noiseFloor_(0.0f), calibrated_(false) {
}

void NoiseCalibrator::startCalibration() {
    std::lock_guard<std::mutex> lock(mutex_);
    rmsSamples_.clear();
    noiseFloor_ = 0.0f;
    calibrated_ = false;
    calibrationStartTime_ = std::chrono::steady_clock::now();
}

bool NoiseCalibrator::addRMSValue(float rms) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (calibrated_) {
        return true;  // Already calibrated
    }
    
    // Check if calibration window has elapsed
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - calibrationStartTime_).count();
    
    if (elapsed >= static_cast<long long>(calibrationDurationMs_)) {
        // Calibration window complete - compute noise floor
        computeNoiseFloor();
        return true;
    }
    
    // Still in calibration window - collect RMS values
    rmsSamples_.push_back(rms);
    return false;
}

bool NoiseCalibrator::isCalibrated() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return calibrated_;
}

float NoiseCalibrator::getNoiseFloor() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return noiseFloor_;
}

void NoiseCalibrator::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    rmsSamples_.clear();
    noiseFloor_ = 0.0f;
    calibrated_ = false;
}

void NoiseCalibrator::computeNoiseFloor() {
    if (rmsSamples_.empty()) {
        // No samples collected - use safety fallback
        noiseFloor_ = 1e-6f;
        calibrated_ = true;
        return;
    }
    
    // Compute average RMS during calibration window
    float sum = std::accumulate(rmsSamples_.begin(), rmsSamples_.end(), 0.0f);
    float avgRms = sum / rmsSamples_.size();
    
    // Set noise floor directly to measured average RMS
    noiseFloor_ = avgRms;
    
    // Safety fallback only if avgRms is extremely small (calibration failure)
    if (avgRms < 1e-6f) {
        noiseFloor_ = 1e-6f;
    }
    
    calibrated_ = true;
}
