#include "audio.h"
#include "network.h"
#include "scene_logger.h"
#include <cmath>
#include <cstdio>  // For FILE, fopen, fclose, fscanf, fprintf
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

// Audio capture state for STT
static HWAVEIN hWaveIn = NULL;
static WAVEFORMATEX wfx = {0};
static WAVEHDR waveHdr[2] = {0};
static std::vector<short> waveBuffer0;  // RAII-managed buffer for waveHdr[0]
static std::vector<short> waveBuffer1;  // RAII-managed buffer for waveHdr[1]
static std::vector<short> capturedSamples;
static bool audioCapturing = false;
static int captureSampleRate = 44100;
static const int CAPTURE_BUFFER_SIZE = 44100; // 1 second of audio at 44.1kHz
static const int SAMPLES_TO_SEND = 44100 * 3; // Send 3 seconds of audio to Whisper

#endif // End of Windows-specific audio capture variables

static int audioSeed = 12345;
static bool audioInitialized = false;
static double soundStartTime = -1.0;
static float soundDuration = 2.0f;
static float soundFadeInDuration = 2.0f;

// Waveform widget state - RMS-based waveform renderer
// Captures audio at 44100 Hz, processes in chunks of 512 samples
// Updates waveform bars at 30fps (every 2 frames at 60fps)
static const int SAMPLE_RATE = 44100;
static const int SAMPLE_BUFFER_SIZE = 512; // Circular buffer for samples
static const int RMS_HISTORY_SIZE = 30; // 1 second of RMS history at 30fps
static const int MAX_BARS = 300; // ~10 seconds at 30fps
static const float CLAMP_THRESHOLD = 0.02f; // Clamp bars below 2% of max
static const float SILENCE_THRESHOLD = 0.001f; // RMS below this is considered silence

// Sample buffer (circular buffer, max 512 samples)
static std::vector<float> sampleBuffer(SAMPLE_BUFFER_SIZE, 0.0f);
static int sampleBufferWriteIndex = 0;
static int sampleBufferCount = 0; // Number of valid samples in buffer

// RMS history (30 values for 1 second at 30fps)
static std::vector<float> rmsHistory;
static float maxRMSSeen = 0.0001f; // Small value to avoid division by zero

// Bar data structure
struct BarData {
    float height; // Normalized height (0.0 to 1.0)
};

// Bar history (300 bars for ~10 seconds)
static std::vector<BarData> barHistory;

// Audio device name (stored during initialization)
static std::string audioDeviceName = "Unknown";

// Update tracking - update at 30fps (every 2 frames at 60fps)
static int frameCount = 0;
static const int UPDATE_INTERVAL_FRAMES = 2; // Update every 2 frames (30fps from 60fps)

// Add audio samples to circular buffer (called from audio callback)
static void updateAudioSamples(const float* samples, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        sampleBuffer[sampleBufferWriteIndex] = samples[i];
        sampleBufferWriteIndex = (sampleBufferWriteIndex + 1) % SAMPLE_BUFFER_SIZE;
        if (sampleBufferCount < SAMPLE_BUFFER_SIZE) {
            sampleBufferCount++;
        }
    }
}

// Calculate RMS from current sample buffer
static float calculateRMS() {
    if (sampleBufferCount == 0) return 0.0f;
    
    float sumSquared = 0.0f;
    for (int i = 0; i < sampleBufferCount; i++) {
        float sample = sampleBuffer[i];
        sumSquared += sample * sample;
    }
    
    return sqrtf(sumSquared / sampleBufferCount);
}

// Add a new bar to history
static void addBar(float heightPercent) {
    BarData bar;
    bar.height = heightPercent;
    
    barHistory.insert(barHistory.begin(), bar); // Insert at front (newest first)
    
    // Keep only MAX_BARS
    if (barHistory.size() > MAX_BARS) {
        barHistory.resize(MAX_BARS);
    }
}

// Simple PRNG based on seed
static float randomFloat(int& seed) {
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return (float)seed / 0x7fffffff;
}

// Generate procedural bass waveform
static void generateBassWaveform(short* buffer, int samples, int sampleRate, int& seed) {
    float phase = 0.0f;
    float freq = 60.0f + randomFloat(seed) * 40.0f; // 60-100 Hz bass
    
    for (int i = 0; i < samples; i++) {
        float time = (float)i / sampleRate;
        float amplitude = 0.0f;
        
        // Fade-in
        if (time < soundFadeInDuration) {
            amplitude = time / soundFadeInDuration;
        } else {
            amplitude = 1.0f;
        }
        
        // Generate waveform: mix sine and low-frequency noise
        float sine = sinf(phase * 2.0f * 3.14159f);
        float noise = (randomFloat(seed) - 0.5f) * 0.2f;
        
        // Low-pass filter the noise
        static float filteredNoise = 0.0f;
        filteredNoise = filteredNoise * 0.9f + noise * 0.1f;
        
        float sample = (sine * 0.7f + filteredNoise * 0.3f) * amplitude;
        
        // Clamp and convert to 16-bit
        sample = fmaxf(-1.0f, fminf(1.0f, sample));
        buffer[i] = (short)(sample * 32767.0f);
        
        phase += freq / sampleRate;
        if (phase > 1.0f) phase -= 1.0f;
    }
}

void initAudioGeneration(int seed) {
    audioSeed = seed;
    audioInitialized = true;
    srand(seed);
}

void playBassSound(float duration, float fadeInDuration) {
    if (!audioInitialized) return;
    
    soundDuration = duration;
    soundFadeInDuration = fadeInDuration;
    soundStartTime = -1.0; // Will be set when actually playing
    
    // Audio playback is currently stubbed - actual implementation would use
    // waveOut API or a proper audio library
    // For now, just mark that sound should play (actual playback not implemented)
    
    #ifdef _WIN32
    // Stub implementation - audio playback not yet fully implemented
    // This prevents crashes from uninitialized audio code
    #endif
}

void updateAudio(float deltaTime) {
    // Update waveform bars at 30fps (every 2 frames at 60fps)
    frameCount++;
    
    // Only update every UPDATE_INTERVAL_FRAMES frames (30fps from 60fps)
    if (frameCount % UPDATE_INTERVAL_FRAMES != 0) {
#ifdef _WIN32
        // Still need to send audio to Whisper STT
        static double accumulatedTime = 0.0;
        accumulatedTime += deltaTime;
        static double lastSTTSend = 0.0;
        if (audioCapturing && accumulatedTime - lastSTTSend >= 3.0) {
            lastSTTSend = accumulatedTime;
            std::vector<short> samplesToSend = capturedSamples;
            if (samplesToSend.size() >= SAMPLES_TO_SEND) {
                samplesToSend.erase(samplesToSend.begin(), samplesToSend.begin() + (samplesToSend.size() - SAMPLES_TO_SEND));
            }
            if (!samplesToSend.empty()) {
                std::cout << "[DEBUG] Audio: Sending " << samplesToSend.size() << " samples to Whisper STT" << std::endl;
                sendAudioToWhisper(samplesToSend, captureSampleRate, "localhost", 8070);
            }
        }
#endif
        return;
    }
    
    // Only process if we have real audio data
    // Don't generate simulated data - if no audio is capturing, just skip update
#ifdef _WIN32
    if (!audioCapturing) {
        // No audio capturing - don't update waveform, just return
        return;
    }
#endif
    
    // Calculate RMS from current sample buffer (from real microphone)
    float rms = calculateRMS();
    
    // Silence detection: if RMS is below threshold, treat as silence
    if (rms < SILENCE_THRESHOLD) {
        rms = 0.0f; // Set to zero for silence
    }
    
    // Add RMS to history
    rmsHistory.insert(rmsHistory.begin(), rms);
    if (rmsHistory.size() > RMS_HISTORY_SIZE) {
        rmsHistory.resize(RMS_HISTORY_SIZE);
    }
    
    // Track max RMS from history
    maxRMSSeen = 0.0001f; // Reset to small value
    for (float h : rmsHistory) {
        if (h > maxRMSSeen) {
            maxRMSSeen = h;
        }
    }
    
    // Calculate heightPercent = rms / maxRMSSeen (0.0 to 1.0)
    float heightPercent = (maxRMSSeen > 0.0001f) ? (rms / maxRMSSeen) : 0.0f;
    
    // Apply clamp threshold: if heightPercent < clampPercent, set to 0
    if (heightPercent < CLAMP_THRESHOLD) {
        heightPercent = 0.0f;
    }
    
    // Calculate bar height (heightPercent * 1.6 as per spec, clamped to 1.0)
    float barHeight = fminf(heightPercent * 1.6f, 1.0f);
    
    // Add new bar to history
    addBar(barHeight);
    
#ifdef _WIN32
    // Send captured audio to Whisper STT every 3 seconds
    static double accumulatedTime = 0.0;
    accumulatedTime += deltaTime * UPDATE_INTERVAL_FRAMES; // Account for frame skip
    static double lastSTTSend = 0.0;
    if (audioCapturing && accumulatedTime - lastSTTSend >= 3.0) {
        lastSTTSend = accumulatedTime;
        std::vector<short> samplesToSend = capturedSamples;
        if (samplesToSend.size() >= SAMPLES_TO_SEND) {
            samplesToSend.erase(samplesToSend.begin(), samplesToSend.begin() + (samplesToSend.size() - SAMPLES_TO_SEND));
        }
        if (!samplesToSend.empty()) {
            std::cout << "[DEBUG] Audio: Sending " << samplesToSend.size() << " samples to Whisper STT" << std::endl;
            sendAudioToWhisper(samplesToSend, captureSampleRate, "localhost", 8070);
        }
    }
#endif
}

std::vector<float> getWaveformAmplitudes() {
    // Return bar heights from history (convert BarData to float heights)
    std::vector<float> heights;
    heights.reserve(barHistory.size());
    for (const auto& bar : barHistory) {
        heights.push_back(bar.height);
    }
    return heights;
}

// Get audio device name
std::string getAudioDeviceName() {
    return audioDeviceName;
}

void cleanupAudio() {
#ifdef _WIN32
    cleanupAudioCapture();
#else
    // Audio capture not supported on non-Windows
#endif
    audioInitialized = false;
}

int getAudioSeed() {
    return audioSeed;
}

void setAudioSeed(int seed) {
    audioSeed = seed;
    if (audioInitialized) {
        srand(seed);
    }
}

bool saveAudioSeed(const std::string& filename) {
    /**
     * Use C file I/O instead of std::ofstream for consistency
     * C file I/O is more stable and avoids potential crashes
     */
    FILE* file = fopen(filename.c_str(), "w");
    if (!file) {
        return false;
    }
    
    /**
     * Write seed value to file
     * Single integer value per line
     */
    int result = fprintf(file, "%d\n", audioSeed);
    fclose(file);
    
    return (result > 0);
}

bool loadAudioSeed(const std::string& filename) {
    /**
     * Use C file I/O instead of std::ifstream to avoid crashes
     * std::ifstream was causing segmentation faults during initialization
     * C file I/O is more stable on Windows
     */
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        return false;
    }
    
    /**
     * Read seed value from file
     * File should contain a single integer value
     * If read fails, use default seed
     */
    int result = fscanf(file, "%d", &audioSeed);
    fclose(file);
    
    if (result != 1) {
        // Failed to read valid integer, use default
        audioSeed = 12345;
        return false;
    }
    
    /**
     * If audio is already initialized, update random seed
     * This ensures new seed takes effect immediately
     */
    if (audioInitialized) {
        srand(audioSeed);
    }
    return true;
}

#ifdef _WIN32
// Windows audio capture functions (continued from line 10)
// All common functions are defined above (lines 85-247)

// Windows audio capture callback
static void CALLBACK waveInProc(HWAVEIN hWaveIn, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WIM_DATA) {
        WAVEHDR* pwh = (WAVEHDR*)dwParam1;
        if (pwh && pwh->dwBytesRecorded > 0) {
            // Add captured samples to buffer
            int numSamples = pwh->dwBytesRecorded / sizeof(short);
            short* samples = (short*)pwh->lpData;
            capturedSamples.insert(capturedSamples.end(), samples, samples + numSamples);
            
            // Keep buffer size manageable - keep last SAMPLES_TO_SEND samples
            if (capturedSamples.size() > SAMPLES_TO_SEND) {
                capturedSamples.erase(capturedSamples.begin(), capturedSamples.begin() + (capturedSamples.size() - SAMPLES_TO_SEND));
            }
            
            // Convert captured short samples to float32 and add to circular buffer
            // This feeds the RMS-based waveform system
            if (numSamples > 0) {
                std::vector<float> floatSamples(numSamples);
                for (int i = 0; i < numSamples; i++) {
                    // Convert 16-bit signed integer to float32 normalized range [-1.0, 1.0]
                    floatSamples[i] = (float)samples[i] / 32768.0f;
                }
                // Add samples to circular buffer for RMS calculation
                updateAudioSamples(floatSamples.data(), numSamples);
                
                // Log periodically to verify real audio is being captured
                static int callbackCount = 0;
                callbackCount++;
                if (callbackCount % 100 == 0) {
                    // Log every 100 callbacks (roughly every few seconds)
                    float currentRMS = calculateRMS();
                    logAudio("Audio callback: " + std::to_string(numSamples) + " samples, RMS: " + std::to_string(currentRMS));
                }
            }
        }
        
        // Re-add buffer for continuous capture
        if (audioCapturing && pwh) {
            waveInUnprepareHeader(hWaveIn, pwh, sizeof(WAVEHDR));
            waveInPrepareHeader(hWaveIn, pwh, sizeof(WAVEHDR));
            waveInAddBuffer(hWaveIn, pwh, sizeof(WAVEHDR));
        }
    }
}

bool initAudioCapture(int sampleRate) {
    if (hWaveIn != NULL) {
        return true; // Already initialized
    }
    
    captureSampleRate = sampleRate;
    
    // Set up WAVEFORMATEX structure
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1; // Mono
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;
    
    // Get number of audio input devices and log default device info
    UINT numDevices = waveInGetNumDevs();
    std::cout << "[DEBUG] Audio: Found " << numDevices << " audio input device(s)" << std::endl;
    
    // Get and store default audio device info
    audioDeviceName = "Unknown Device";
    if (numDevices > 0) {
        WAVEINCAPSA wic;
        MMRESULT capsResult = waveInGetDevCapsA(WAVE_MAPPER, &wic, sizeof(WAVEINCAPSA));
        if (capsResult == MMSYSERR_NOERROR) {
            // szPname is already CHAR* (not wide) in WAVEINCAPSA
            audioDeviceName = std::string(wic.szPname);
            std::cout << "[DEBUG] Audio: Using default device: " << audioDeviceName << std::endl;
            std::cout << "[DEBUG] Audio: Device supports " << (int)wic.wChannels << " channels" << std::endl;
            
            // Log to audio.log
            logAudio("Audio capture initialized");
            logAudio("Found " + std::to_string(numDevices) + " audio input device(s)");
            logAudio("Using device: " + audioDeviceName);
            logAudio("Device supports " + std::to_string((int)wic.wChannels) + " channels");
            logAudio("Sample rate: " + std::to_string(sampleRate) + " Hz");
        }
    } else {
        logAudio("Audio capture initialized - No audio devices found");
    }
    
    // Open wave input device
    MMRESULT result = waveInOpen(&hWaveIn, WAVE_MAPPER, &wfx, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "[ERROR] Audio: waveInOpen failed: " << result << std::endl;
        return false;
    }
    
    // Allocate buffers using std::vector (RAII-managed)
    waveBuffer0.resize(CAPTURE_BUFFER_SIZE);
    waveBuffer1.resize(CAPTURE_BUFFER_SIZE);
    
    // Prepare buffers
    waveHdr[0].lpData = (LPSTR)waveBuffer0.data();
    waveHdr[0].dwBufferLength = CAPTURE_BUFFER_SIZE * sizeof(short);
    waveHdr[0].dwFlags = 0;
    
    waveHdr[1].lpData = (LPSTR)waveBuffer1.data();
    waveHdr[1].dwBufferLength = CAPTURE_BUFFER_SIZE * sizeof(short);
    waveHdr[1].dwFlags = 0;
    
    for (int i = 0; i < 2; i++) {
        result = waveInPrepareHeader(hWaveIn, &waveHdr[i], sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            std::cerr << "[ERROR] Audio: waveInPrepareHeader failed: " << result << std::endl;
            waveInClose(hWaveIn);
            hWaveIn = NULL;
            return false;
        }
    }
    
    std::cout << "[DEBUG] Audio: Capture initialized at " << sampleRate << "Hz" << std::endl;
    return true;
}

void cleanupAudioCapture() {
    stopAudioCapture();
    
    if (hWaveIn) {
        // Buffers are RAII-managed by std::vector - no manual free needed
        // Just clear the pointers
        for (int i = 0; i < 2; i++) {
            waveHdr[i].lpData = NULL;
        }
        
        waveInClose(hWaveIn);
        hWaveIn = NULL;
    }
    
    // Clear RAII buffers
    waveBuffer0.clear();
    waveBuffer1.clear();
    capturedSamples.clear();
    std::cout << "[DEBUG] Audio: Capture cleaned up" << std::endl;
}

void startAudioCapture() {
    if (!hWaveIn || audioCapturing) {
        return;
    }
    
    // Add buffers to input queue
    for (int i = 0; i < 2; i++) {
        MMRESULT result = waveInAddBuffer(hWaveIn, &waveHdr[i], sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            std::cerr << "[ERROR] Audio: waveInAddBuffer failed: " << result << std::endl;
            return;
        }
    }
    
    // Start recording
    MMRESULT result = waveInStart(hWaveIn);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "[ERROR] Audio: waveInStart failed: " << result << std::endl;
        return;
    }
    
    audioCapturing = true;
    capturedSamples.clear();
    std::cout << "[DEBUG] Audio: Capture started" << std::endl;
}

void stopAudioCapture() {
    if (!hWaveIn || !audioCapturing) {
        return;
    }
    
    waveInStop(hWaveIn);
    waveInReset(hWaveIn);
    
    // Unprepare headers
    for (int i = 0; i < 2; i++) {
        waveInUnprepareHeader(hWaveIn, &waveHdr[i], sizeof(WAVEHDR));
    }
    
    audioCapturing = false;
    std::cout << "[DEBUG] Audio: Capture stopped" << std::endl;
}

bool isAudioCapturing() {
    return audioCapturing;
}

std::vector<short> getCapturedAudioSamples() {
    return capturedSamples;
}

#else
// Non-Windows stub implementation - only platform-specific functions here
bool initAudioCapture(int sampleRate) {
    return false;
}

void cleanupAudioCapture() {
    // No-op on non-Windows
}

void startAudioCapture() {
    // No-op on non-Windows
}

void stopAudioCapture() {
    // No-op on non-Windows
}

bool isAudioCapturing() {
    return false;
}

std::vector<short> getCapturedAudioSamples() {
    return std::vector<short>();
}

#endif // _WIN32
