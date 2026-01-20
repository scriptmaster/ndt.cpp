#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Include the new audio capture architecture
#include "AudioCapture/AudioCaptureOrchestrator.h"

/**
 * Audio Device Test - Using production-quality audio capture architecture
 * 
 * Tests the complete audio pipeline:
 * 1. RMS calculation over fixed windows
 * 2. Automatic noise floor calibration
 * 3. Speech/silence detection with hysteresis
 * 4. PCM buffering for future STT integration
 */

// ============================================================================
// Platform-specific audio capture (Windows)
// ============================================================================

class WindowsAudioCapture {
public:
    WindowsAudioCapture() : hWaveIn_(NULL), capturing_(false), sampleRate_(44100) {
        memset(&wfx_, 0, sizeof(WAVEFORMATEX));
    }
    
    ~WindowsAudioCapture() {
        cleanup();
    }
    
    bool init(int sampleRate) {
        if (hWaveIn_ != NULL) {
            return true;
        }
        
        sampleRate_ = sampleRate;
        wfx_.wFormatTag = WAVE_FORMAT_PCM;
        wfx_.nChannels = 1;
        wfx_.nSamplesPerSec = sampleRate;
        wfx_.wBitsPerSample = 16;
        wfx_.nBlockAlign = wfx_.nChannels * (wfx_.wBitsPerSample / 8);
        wfx_.nAvgBytesPerSec = wfx_.nSamplesPerSec * wfx_.nBlockAlign;
        wfx_.cbSize = 0;
        
        UINT numDevices = waveInGetNumDevs();
        if (numDevices == 0) {
            std::cerr << "ERROR: No audio devices found!" << std::endl;
            return false;
        }
        
    // Get device info for WAVE_MAPPER (default device)
    WAVEINCAPSA wic;
    MMRESULT capsResult = waveInGetDevCapsA(WAVE_MAPPER, &wic, sizeof(WAVEINCAPSA));
    if (capsResult == MMSYSERR_NOERROR) {
        deviceName_ = std::string(wic.szPname);
        std::cout << "\n========================================" << std::endl;
        std::cout << "Selected Device (WAVE_MAPPER - Default Input):" << std::endl;
        std::cout << "  Device ID: " << WAVE_MAPPER << std::endl;
        std::cout << "  Name: " << deviceName_ << std::endl;
        std::cout << "  Channels: " << (int)wic.wChannels << std::endl;
        std::cout << "  Sample rate: " << sampleRate << " Hz" << std::endl;
        std::cout << "========================================\n" << std::endl;
    }
        
        // Open for actual capture
        result = waveInOpen(&hWaveIn_, WAVE_MAPPER, &wfx_, (DWORD_PTR)waveInProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
        if (result != MMSYSERR_NOERROR) {
            std::cerr << "ERROR: waveInOpen failed: " << result << std::endl;
            return false;
        }
        
        const int CAPTURE_BUFFER_SIZE = 44100;
        for (int i = 0; i < 2; i++) {
            waveHdr_[i].lpData = (LPSTR)malloc(CAPTURE_BUFFER_SIZE * sizeof(short));
            waveHdr_[i].dwBufferLength = CAPTURE_BUFFER_SIZE * sizeof(short);
            waveHdr_[i].dwFlags = 0;
            
            result = waveInPrepareHeader(hWaveIn_, &waveHdr_[i], sizeof(WAVEHDR));
            if (result != MMSYSERR_NOERROR) {
                std::cerr << "ERROR: waveInPrepareHeader failed: " << result << std::endl;
                cleanup();
                return false;
            }
        }
        
        return true;
    }
    
    void start() {
        if (!hWaveIn_ || capturing_) {
            return;
        }
        
        for (int i = 0; i < 2; i++) {
            MMRESULT result = waveInAddBuffer(hWaveIn_, &waveHdr_[i], sizeof(WAVEHDR));
            if (result != MMSYSERR_NOERROR) {
                std::cerr << "ERROR: waveInAddBuffer failed: " << result << std::endl;
                return;
            }
        }
        
        MMRESULT result = waveInStart(hWaveIn_);
        if (result != MMSYSERR_NOERROR) {
            std::cerr << "ERROR: waveInStart failed: " << result << std::endl;
            return;
        }
        
        capturing_ = true;
        std::cout << "Audio capture started..." << std::endl;
    }
    
    void stop() {
        if (!hWaveIn_ || !capturing_) {
            return;
        }
        
        waveInStop(hWaveIn_);
        waveInReset(hWaveIn_);
        
        for (int i = 0; i < 2; i++) {
            waveInUnprepareHeader(hWaveIn_, &waveHdr_[i], sizeof(WAVEHDR));
        }
        
        capturing_ = false;
        std::cout << "Audio capture stopped." << std::endl;
    }
    
    void cleanup() {
        stop();
        
        if (hWaveIn_) {
            for (int i = 0; i < 2; i++) {
                if (waveHdr_[i].lpData) {
                    free(waveHdr_[i].lpData);
                    waveHdr_[i].lpData = NULL;
                }
            }
            
            waveInClose(hWaveIn_);
            hWaveIn_ = NULL;
        }
    }
    
    // Get captured samples (thread-safe)
    std::vector<int16_t> getSamples() {
        std::lock_guard<std::mutex> lock(samplesMutex_);
        std::vector<int16_t> result = capturedSamples_;
        capturedSamples_.clear();
        return result;
    }
    
    int getSampleRate() const { return sampleRate_; }
    std::string getDeviceName() const { return deviceName_; }
    bool isCapturing() const { return capturing_; }
    
    // Callback for new samples
    void onSamplesReceived(const int16_t* samples, int numSamples) {
        std::lock_guard<std::mutex> lock(samplesMutex_);
        capturedSamples_.insert(capturedSamples_.end(), samples, samples + numSamples);
    }
    
    // Static callback wrapper
    static void CALLBACK waveInProc(HWAVEIN hWaveIn, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR /* dwParam2 */) {
        if (uMsg == WIM_DATA) {
            WindowsAudioCapture* self = (WindowsAudioCapture*)dwInstance;
            WAVEHDR* pwh = (WAVEHDR*)dwParam1;
            if (self && pwh && pwh->dwBytesRecorded > 0) {
                int numSamples = pwh->dwBytesRecorded / sizeof(int16_t);
                int16_t* samples = (int16_t*)pwh->lpData;
                self->onSamplesReceived(samples, numSamples);
                
                // Re-add buffer for continuous capture
                if (self->capturing_ && pwh) {
                    waveInUnprepareHeader(hWaveIn, pwh, sizeof(WAVEHDR));
                    waveInPrepareHeader(hWaveIn, pwh, sizeof(WAVEHDR));
                    waveInAddBuffer(hWaveIn, pwh, sizeof(WAVEHDR));
                }
            }
        }
    }
    
private:
    HWAVEIN hWaveIn_;
    WAVEFORMATEX wfx_;
    WAVEHDR waveHdr_[2];
    std::vector<int16_t> capturedSamples_;
    std::mutex samplesMutex_;
    bool capturing_;
    int sampleRate_;
    std::string deviceName_;
};

// ============================================================================
// Device enumeration
// ============================================================================

void listAllAudioDevices() {
    UINT numDevices = waveInGetNumDevs();
    std::cout << "========================================" << std::endl;
    std::cout << "Audio Input Devices Found: " << numDevices << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (numDevices == 0) {
        std::cout << "No audio input devices found!" << std::endl;
        return;
    }
    
    for (UINT i = 0; i < numDevices; i++) {
        WAVEINCAPSA wic;
        MMRESULT capsResult = waveInGetDevCapsA(i, &wic, sizeof(WAVEINCAPSA));
        if (capsResult == MMSYSERR_NOERROR) {
            std::string deviceName = std::string(wic.szPname);
            std::cout << "\nDevice " << i << ":" << std::endl;
            std::cout << "  Name: " << deviceName << std::endl;
            std::cout << "  Channels: " << (int)wic.wChannels << std::endl;
            std::cout << "  Manufacturer ID: " << wic.wMid << std::endl;
            std::cout << "  Product ID: " << wic.wPid << std::endl;
        } else {
            std::cerr << "  ERROR: Failed to get caps for device " << i << ": " << capsResult << std::endl;
        }
    }
    std::cout << "\n========================================\n" << std::endl;
}

// ============================================================================
// Main test program
// ============================================================================

int main() {
    std::cout << "[DEBUG] Program starting..." << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Audio Device Test (Production Architecture)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nThis test demonstrates:" << std::endl;
    std::cout << "1. RMS calculation over fixed 100ms windows" << std::endl;
    std::cout << "2. Automatic noise floor calibration (300ms)" << std::endl;
    std::cout << "3. Speech detection with hysteresis" << std::endl;
    std::cout << "4. PCM buffering for future STT integration" << std::endl;
    std::cout << "\nPress Enter to start..." << std::endl;
    std::cin.get();
    
    // List devices
    listAllAudioDevices();
    
    // Initialize components
    const int SAMPLE_RATE = 44100;
    WindowsAudioCapture capture;
    AudioCaptureOrchestrator orchestrator(SAMPLE_RATE);
    
    // Statistics
    int speechSegmentCount = 0;
    size_t totalSamplesBuffered = 0;
    
    // Set up callbacks
    orchestrator.setOnSegmentReady([&](const std::vector<int16_t>& segment) {
        speechSegmentCount++;
        totalSamplesBuffered += segment.size();
        double durationMs = (segment.size() * 1000.0) / SAMPLE_RATE;
        
        std::cout << "\n[SEGMENT READY] #" << speechSegmentCount << std::endl;
        std::cout << "  Samples: " << segment.size() << std::endl;
        std::cout << "  Duration: " << std::fixed << std::setprecision(1) << durationMs << "ms" << std::endl;
        std::cout << "  Ready for Whisper integration" << std::endl;
        // TODO: Send to Whisper here
    });
    
    orchestrator.setOnDebugInfo([&](float rms, float noiseFloor, bool isSpeaking, size_t bufferSize) {
        // Debug: validate buffer size
        if (bufferSize > 1000000) {  // Sanity check - buffer should never be this large
            static bool warned = false;
            if (!warned) {
                std::cerr << "[WARNING] Invalid buffer size: " << bufferSize << std::endl;
                warned = true;
            }
        }
    });
    
    // Initialize capture
    if (!capture.init(SAMPLE_RATE)) {
        std::cerr << "Failed to initialize audio capture!" << std::endl;
        return 1;
    }
    
    // Start capture and orchestrator
    capture.start();
    orchestrator.start();
    
    // Configurable logging FPS
    int logFPS = 10;
    const char* envFPS = std::getenv("AUDIO_TEST_FPS");
    if (envFPS) {
        int envFPSValue = std::atoi(envFPS);
        if (envFPSValue >= 1 && envFPSValue <= 60) {
            logFPS = envFPSValue;
        }
    }
    
    int logIntervalMs = 1000 / logFPS;
    
    // Open log file
    std::ofstream logFile("logs/audio_test_rms.log");
    if (!logFile.is_open()) {
        std::cerr << "Warning: Could not open logs/audio_test_rms.log, logging to console only" << std::endl;
    } else {
        logFile << "Timestamp(s),RMS,RMS_Percent,NoiseFloor,IsSpeaking,BufferSize\n";
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Capturing audio for 10 seconds..." << std::endl;
    std::cout << "RMS window: 100ms" << std::endl;
    std::cout << "Noise calibration: 300ms" << std::endl;
    std::cout << "Speech threshold: 2.5x noise floor" << std::endl;
    std::cout << "Silence threshold: 1.5x noise floor" << std::endl;
    std::cout << "Logging at " << logFPS << "fps (" << logIntervalMs << "ms intervals)" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    auto startTime = std::chrono::steady_clock::now();
    auto lastLog = startTime;
    const auto LOG_INTERVAL = std::chrono::milliseconds(logIntervalMs);
    const auto TEST_DURATION = std::chrono::seconds(10);
    
    std::cout << "[DEBUG] Starting main loop" << std::endl;
    while (true) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);

        if (elapsed >= TEST_DURATION) {
            std::cout << "[DEBUG] Test duration reached, exiting loop" << std::endl;
            break;
        }

        // Debug: check orchestrator every second
        static auto lastDebugTime = startTime;
        auto timeSinceDebug = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastDebugTime);
        if (timeSinceDebug.count() >= 1) {
            std::cout << "[DEBUG] Orchestrator buffer size: " << orchestrator.getBufferSize() << std::endl;
            lastDebugTime = currentTime;
        }

        // Get new samples from capture
        std::vector<int16_t> samples = capture.getSamples();
        if (!samples.empty()) {
            // Process samples through orchestrator
            orchestrator.processSamples(samples.data(), samples.size());
        }
        
        // Log at configured interval
        auto timeSinceLastLog = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastLog);
        if (timeSinceLastLog >= LOG_INTERVAL) {
            float rms = orchestrator.getCurrentRMS();
            float noiseFloor = orchestrator.getNoiseFloor();
            bool isSpeaking = orchestrator.isSpeaking();
            size_t bufferSize = orchestrator.getBufferSize();
            bool calibrated = orchestrator.isCalibrated();
            
            double timestamp = elapsed.count() / 1000.0;
            float rmsPercent = (rms / 0.1f) * 100.0f;
            if (rmsPercent > 100.0f) rmsPercent = 100.0f;
            
            std::cout << "[" << std::fixed << std::setprecision(3) << timestamp << "s] "
                      << "RMS: " << std::setprecision(6) << rms << " (" 
                      << std::setprecision(2) << rmsPercent << "%) ";
            
            if (calibrated) {
                std::cout << "Noise: " << std::setprecision(6) << noiseFloor << " ";
            } else {
                std::cout << "[CALIBRATING] ";
            }
            
            std::cout << (isSpeaking ? "[SPEECH]" : "[SILENCE]")
                      << " Buffer: " << bufferSize << " samples" << std::endl;
            
            if (logFile.is_open()) {
                // Sanitize bufferSize for logging
                size_t logBufferSize = (bufferSize > 1000000) ? 0 : bufferSize;
                logFile << timestamp << "," << rms << "," << rmsPercent << ","
                        << noiseFloor << "," << (isSpeaking ? "1" : "0") << ","
                        << logBufferSize << "\n";
            }
            
            lastLog = currentTime;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Stop capture and orchestrator
    orchestrator.stop();
    capture.stop();
    capture.cleanup();
    
    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Complete!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total speech segments detected: " << speechSegmentCount << std::endl;
    std::cout << "Total samples buffered: " << totalSamplesBuffered << std::endl;
    std::cout << "Noise floor: " << orchestrator.getNoiseFloor() << std::endl;
    
    if (logFile.is_open()) {
        logFile.close();
        std::cout << "\nRMS values logged to: logs/audio_test_rms.log" << std::endl;
    }
    
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}

#else
// Non-Windows stub
int main() {
    std::cout << "Audio device test is only available on Windows" << std::endl;
    return 1;
}
#endif
