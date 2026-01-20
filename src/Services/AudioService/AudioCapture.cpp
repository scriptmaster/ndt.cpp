#include "AudioCapture.h"
#include "../LoggingService/SceneLogger.h"
#include <iostream>
#include <vector>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Audio capture state
static HWAVEIN hWaveIn = NULL;
static WAVEFORMATEX wfx = {0};
static WAVEHDR waveHdr[2] = {0};
static std::vector<short> capturedSamples;
static bool audioCapturing = false;
static int captureSampleRate = 44100;
static const int CAPTURE_BUFFER_SIZE = 44100;
static const int SAMPLES_TO_SEND = 44100 * 3;

// Audio device name (stored during initialization)
static std::string audioDeviceName = "Unknown";

// Forward declarations (implemented in AudioWaveform.cpp and SceneLogger.cpp)
extern void updateAudioSamples(const float* samples, int numSamples);
extern void logAudio(const std::string& message);

// Windows audio capture callback
static void CALLBACK waveInProc(HWAVEIN hWaveIn, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WIM_DATA) {
        WAVEHDR* pwh = (WAVEHDR*)dwParam1;
        if (pwh && pwh->dwBytesRecorded > 0) {
            int numSamples = pwh->dwBytesRecorded / sizeof(short);
            short* samples = (short*)pwh->lpData;
            capturedSamples.insert(capturedSamples.end(), samples, samples + numSamples);
            
            if (capturedSamples.size() > SAMPLES_TO_SEND) {
                capturedSamples.erase(capturedSamples.begin(), capturedSamples.begin() + (capturedSamples.size() - SAMPLES_TO_SEND));
            }
            
            if (numSamples > 0) {
                std::vector<float> floatSamples(numSamples);
                for (int i = 0; i < numSamples; i++) {
                    floatSamples[i] = (float)samples[i] / 32768.0f;
                }
                updateAudioSamples(floatSamples.data(), numSamples);
            }
        }
        
        if (audioCapturing && pwh) {
            waveInUnprepareHeader(hWaveIn, pwh, sizeof(WAVEHDR));
            waveInPrepareHeader(hWaveIn, pwh, sizeof(WAVEHDR));
            waveInAddBuffer(hWaveIn, pwh, sizeof(WAVEHDR));
        }
    }
}

void listAllAudioDevices() {
    UINT numDevices = waveInGetNumDevs();
    std::cout << "[DEBUG] Audio: Found " << numDevices << " audio input device(s)" << std::endl;
    logAudio("Found " + std::to_string(numDevices) + " audio input device(s)");
    
    // Enumerate ALL devices (not just default)
    for (UINT i = 0; i < numDevices; i++) {
        WAVEINCAPSA wic;
        MMRESULT capsResult = waveInGetDevCapsA(i, &wic, sizeof(WAVEINCAPSA));
        if (capsResult == MMSYSERR_NOERROR) {
            std::string deviceName = std::string(wic.szPname);
            std::cout << "[DEBUG] Audio: Device " << i << ": " << deviceName << std::endl;
            std::cout << "[DEBUG] Audio:   - Channels: " << (int)wic.wChannels << std::endl;
            std::cout << "[DEBUG] Audio:   - Manufacturer ID: " << wic.wMid << std::endl;
            std::cout << "[DEBUG] Audio:   - Product ID: " << wic.wPid << std::endl;
            
            logAudio("Device " + std::to_string(i) + ": " + deviceName);
            logAudio("  Channels: " + std::to_string((int)wic.wChannels));
            logAudio("  Manufacturer ID: " + std::to_string(wic.wMid));
            logAudio("  Product ID: " + std::to_string(wic.wPid));
        } else {
            std::cerr << "[ERROR] Audio: Failed to get caps for device " << i << ": " << capsResult << std::endl;
        }
    }
}

bool initAudioCapture(int sampleRate) {
    if (hWaveIn != NULL) {
        return true;
    }
    
    captureSampleRate = sampleRate;
    
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;
    
    // List ALL audio devices first
    listAllAudioDevices();
    
    // Get and store default audio device info
    audioDeviceName = "Unknown Device";
    UINT numDevices = waveInGetNumDevs();
    if (numDevices > 0) {
        WAVEINCAPSA wic;
        MMRESULT capsResult = waveInGetDevCapsA(WAVE_MAPPER, &wic, sizeof(WAVEINCAPSA));
        if (capsResult == MMSYSERR_NOERROR) {
            audioDeviceName = std::string(wic.szPname);
            std::cout << "[DEBUG] Audio: Using default device: " << audioDeviceName << std::endl;
            std::cout << "[DEBUG] Audio: Device supports " << (int)wic.wChannels << " channels" << std::endl;
            
            logAudio("Using device: " + audioDeviceName);
            logAudio("Device supports " + std::to_string((int)wic.wChannels) + " channels");
            logAudio("Sample rate: " + std::to_string(sampleRate) + " Hz");
        }
    } else {
        logAudio("Audio capture initialized - No audio devices found");
    }
    
    MMRESULT result = waveInOpen(&hWaveIn, WAVE_MAPPER, &wfx, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "[ERROR] Audio: waveInOpen failed: " << result << std::endl;
        return false;
    }
    
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
    
    for (int i = 0; i < 2; i++) {
        MMRESULT result = waveInAddBuffer(hWaveIn, &waveHdr[i], sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            std::cerr << "[ERROR] Audio: waveInAddBuffer failed: " << result << std::endl;
            return;
        }
    }
    
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

std::string getAudioDeviceName() {
    return audioDeviceName;
}

#else
// Non-Windows stub implementation
bool initAudioCapture(int sampleRate) {
    return false;
}

void cleanupAudioCapture() {
}

void startAudioCapture() {
}

void stopAudioCapture() {
}

bool isAudioCapturing() {
    return false;
}

std::vector<short> getCapturedAudioSamples() {
    return std::vector<short>();
}

void listAllAudioDevices() {
}

std::string getAudioDeviceName() {
    return "Unknown (non-Windows)";
}
#endif
