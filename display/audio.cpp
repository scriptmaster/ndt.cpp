#include "audio.h"
#include "network.h"
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

// Waveform widget state - amplitude values updated every 100ms
// 44100 samples/sec / 10 bars/sec = 4410 samples per bar (per 100ms)
// 10 bars total (one second worth), rendered at 60fps
static const int SAMPLE_RATE = 44100;
static const int SAMPLES_PER_BAR = SAMPLE_RATE / 10; // 4410 samples per 100ms bar
static const int NUM_BARS = 10; // 10 bars per second (100ms intervals)
static std::vector<float> waveformAmplitudes(NUM_BARS, 0.0f); // 10 bars
static double lastWaveformUpdate = 0.0;
static const double WAVEFORM_UPDATE_INTERVAL = 0.1; // 100ms

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

void initAudio(int seed) {
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
    // Update waveform amplitudes every 100ms using deltaTime accumulation
    // Each bar represents 4410 samples (100ms at 44100 samples/sec) averaged
    static double accumulatedTime = 0.0;
    accumulatedTime += deltaTime;
    
    if (accumulatedTime - lastWaveformUpdate >= WAVEFORM_UPDATE_INTERVAL) {
        lastWaveformUpdate = accumulatedTime;
        
        // If capturing real audio, waveform is updated in waveInProc callback
        // Otherwise, use simulated waveform
#ifdef _WIN32
        if (!audioCapturing) {
#endif
            // Shift bars to the left (oldest bar is removed)
            for (int i = NUM_BARS - 1; i > 0; i--) {
                waveformAmplitudes[i] = waveformAmplitudes[i - 1];
            }
            
            // Generate new bar value by averaging 4410 samples (simulated)
            static float phase = 0.0f;
            phase += 0.1f; // Advance phase by 100ms
            if (phase > 100.0f) phase -= 100.0f;
            
            float sumAmplitude = 0.0f;
            int localSeed = audioSeed;
            
            // Average 4410 samples to get one bar value
            for (int i = 0; i < SAMPLES_PER_BAR; i++) {
                float t = (float)i / SAMPLES_PER_BAR + phase;
                // Generate amplitude using sine waves at different frequencies
                float amp1 = sinf(t * 2.0f * 3.14159f * 2.0f) * 0.5f + 0.5f; // Low frequency
                float amp2 = sinf(t * 2.0f * 3.14159f * 5.0f) * 0.3f + 0.7f; // Mid frequency
                float amp3 = ((localSeed % 100) / 100.0f) * 0.2f; // Random noise based on seed
                localSeed = (localSeed * 1103515245 + 12345) & 0x7fffffff;
                float sample = (amp1 * 0.5f + amp2 * 0.3f + amp3 * 0.2f);
                sumAmplitude += sample;
            }
            
            // Average the samples and store as new bar (leftmost position)
            waveformAmplitudes[0] = sumAmplitude / SAMPLES_PER_BAR;
#ifdef _WIN32
        }
        
        // Send captured audio to Whisper STT every 3 seconds
        static double lastSTTSend = 0.0;
        if (audioCapturing && accumulatedTime - lastSTTSend >= 3.0) {
            lastSTTSend = accumulatedTime;
            
            // Get captured samples (last 3 seconds)
            std::vector<short> samplesToSend = capturedSamples;
            if (samplesToSend.size() >= SAMPLES_TO_SEND) {
                // Take last 3 seconds
                samplesToSend.erase(samplesToSend.begin(), samplesToSend.begin() + (samplesToSend.size() - SAMPLES_TO_SEND));
            }
            
            // Send to Whisper STT on port 8070
            if (!samplesToSend.empty()) {
                std::cout << "[DEBUG] Audio: Sending " << samplesToSend.size() << " samples to Whisper STT" << std::endl;
                sendAudioToWhisper(samplesToSend, captureSampleRate, "localhost", 8070);
            }
        }
#endif
    }
}

std::vector<float> getWaveformAmplitudes() {
    return waveformAmplitudes;
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
            
            // Update waveform amplitudes from real audio data
            if (numSamples > 0) {
                float sumAmplitude = 0.0f;
                float maxAmplitude = 0.0f;
                for (int i = 0; i < numSamples; i++) {
                    float normalized = (float)samples[i] / 32768.0f;
                    sumAmplitude += fabsf(normalized);
                    if (fabsf(normalized) > maxAmplitude) {
                        maxAmplitude = fabsf(normalized);
                    }
                }
                float avgAmplitude = sumAmplitude / numSamples;
                
                // Shift waveform bars and add new one
                for (int i = NUM_BARS - 1; i > 0; i--) {
                    waveformAmplitudes[i] = waveformAmplitudes[i - 1];
                }
                waveformAmplitudes[0] = maxAmplitude; // Use max amplitude for visual effect
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
    
    // Open wave input device
    MMRESULT result = waveInOpen(&hWaveIn, WAVE_MAPPER, &wfx, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "[ERROR] Audio: waveInOpen failed: " << result << std::endl;
        return false;
    }
    
    // Prepare buffers
    for (int i = 0; i < 2; i++) {
        waveHdr[i].lpData = (LPSTR)malloc(CAPTURE_BUFFER_SIZE * sizeof(short));
        waveHdr[i].dwBufferLength = CAPTURE_BUFFER_SIZE * sizeof(short);
        waveHdr[i].dwFlags = 0;
        
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
        // Free buffers
        for (int i = 0; i < 2; i++) {
            if (waveHdr[i].lpData) {
                free(waveHdr[i].lpData);
                waveHdr[i].lpData = NULL;
            }
        }
        
        waveInClose(hWaveIn);
        hWaveIn = NULL;
    }
    
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
