#include "AudioCapture.h"
#include "AudioWaveform.h"
#include <atomic>
#include <iostream>
#include <mutex>
#include <vector>
#include <cmath>
#include "../LoggingService/SceneLogger.h"
#include "AudioCaptureService.h"

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

// Audio capture state
static HWAVEIN hWaveIn = NULL;
static WAVEFORMATEX wfx = {};
static WAVEHDR waveHdr[2] = {};
static std::vector<short> capturedSamples;
static bool audioCapturing = false;
static int captureSampleRate = 44100;
static int captureDeviceIndex = 0;
static const int CAPTURE_FRAME_MS = 20;
static int captureFramesPerBuffer = 0;
static int captureSamplesPerBuffer = 0;
static int samplesToKeep = 0;

// Audio device name (stored during initialization)
static std::string audioDeviceName = "Unknown";
static std::mutex audioMutex;
static std::atomic<int> callbackCount{0};
static std::atomic<int> zeroByteCount{0};
static std::vector<float> monoFloatScratch;
static std::vector<short> monoShortScratch;

static std::string toLower(const std::string& input) {
    std::string out = input;
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return out;
}

int getAudioCaptureDeviceIndex() {
    return captureDeviceIndex;
}

bool setAudioCaptureDeviceIndex(int index) {
    if (index < 0) {
        return false;
    }
    captureDeviceIndex = index;
    if (hWaveIn != NULL) {
        stopAudioCapture();
        cleanupAudioCapture();
        if (!initAudioCapture(captureSampleRate)) {
            return false;
        }
        startAudioCapture();
    }
    return true;
}

int getAudioCaptureCallbackCount() {
    return callbackCount.load(std::memory_order_relaxed);
}

int getAudioCaptureZeroByteCount() {
    return zeroByteCount.load(std::memory_order_relaxed);
}

// Forward declarations (implemented in SceneLogger.cpp)
extern void logAudio(const std::string& message);

// Windows audio capture callback
static void CALLBACK waveInProc(HWAVEIN hWaveIn, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WIM_DATA) {
        (void)dwInstance;
        (void)dwParam2;
        callbackCount.fetch_add(1, std::memory_order_relaxed);
        WAVEHDR* pwh = (WAVEHDR*)dwParam1;
        if (pwh && pwh->dwBytesRecorded > 0) {
            int totalSamples = pwh->dwBytesRecorded / sizeof(short);
            short* samples = (short*)pwh->lpData;
            int frameCount = totalSamples / 2;
            if (frameCount > 0 && frameCount <= static_cast<int>(monoFloatScratch.size())) {
                float* monoFloatSamples = monoFloatScratch.data();
                short* monoSamples = monoShortScratch.data();
                for (int i = 0; i < frameCount; i++) {
                    int left = samples[i * 2];
                    int right = samples[i * 2 + 1];
                    short mono = static_cast<short>((left + right) / 2);
                    monoSamples[i] = mono;
                    monoFloatSamples[i] = static_cast<float>(mono) / 32768.0f;
                }

                {
                    std::lock_guard<std::mutex> lock(audioMutex);
                    if (capturedSamples.size() + static_cast<size_t>(frameCount) > static_cast<size_t>(samplesToKeep)) {
                        size_t excess = capturedSamples.size() + static_cast<size_t>(frameCount) - samplesToKeep;
                        if (excess >= capturedSamples.size()) {
                            capturedSamples.clear();
                        } else {
                            capturedSamples.erase(capturedSamples.begin(),
                                                  capturedSamples.begin() + excess);
                        }
                    }
                    capturedSamples.insert(capturedSamples.end(), monoSamples, monoSamples + frameCount);
                }

                updateAudioSamples(monoFloatSamples, frameCount);

                auto captureService = AudioCaptureService::GetInstance();
                if (captureService) {
                    captureService->EnqueueAudioSamples(monoFloatSamples, frameCount);
                }
            }
        } else {
            zeroByteCount.fetch_add(1, std::memory_order_relaxed);
        }
        
        if (audioCapturing && pwh) {
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
    callbackCount.store(0, std::memory_order_relaxed);
    zeroByteCount.store(0, std::memory_order_relaxed);
    captureFramesPerBuffer = (captureSampleRate * CAPTURE_FRAME_MS) / 1000;
    if (captureFramesPerBuffer < 1) {
        captureFramesPerBuffer = 1;
    }
    captureSamplesPerBuffer = captureFramesPerBuffer * 2; // stereo samples
    samplesToKeep = captureSampleRate * 3; // mono samples for 3 seconds
    monoFloatScratch.resize(captureFramesPerBuffer);
    monoShortScratch.resize(captureFramesPerBuffer);
    {
        std::lock_guard<std::mutex> lock(audioMutex);
        capturedSamples.clear();
        capturedSamples.reserve(samplesToKeep);
    }
    
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
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
        captureDeviceIndex = 0;
        std::string defaultName;
        {
            WAVEINCAPSA defaultCaps;
            MMRESULT defaultResult = waveInGetDevCapsA(WAVE_MAPPER, &defaultCaps, sizeof(WAVEINCAPSA));
            if (defaultResult == MMSYSERR_NOERROR) {
                defaultName = defaultCaps.szPname;
            }
        }

        if (numDevices > 1) {
            int matchedIndex = -1;
            std::string defaultNameLower = toLower(defaultName);
            for (UINT i = 0; i < numDevices; i++) {
                WAVEINCAPSA wicCheck;
                MMRESULT capsResult = waveInGetDevCapsA(i, &wicCheck, sizeof(WAVEINCAPSA));
                if (capsResult != MMSYSERR_NOERROR) {
                    continue;
                }
                std::string deviceName = wicCheck.szPname;
                std::string deviceNameLower = toLower(deviceName);
                if (!defaultNameLower.empty() && deviceNameLower == defaultNameLower) {
                    matchedIndex = static_cast<int>(i);
                    break;
                }
                if (matchedIndex == -1 && deviceNameLower.find("default") != std::string::npos) {
                    matchedIndex = static_cast<int>(i);
                }
            }
            if (matchedIndex >= 0) {
                captureDeviceIndex = matchedIndex;
            }
        }

        WAVEINCAPSA wic;
        MMRESULT capsResult = waveInGetDevCapsA(captureDeviceIndex, &wic, sizeof(WAVEINCAPSA));
        if (capsResult == MMSYSERR_NOERROR) {
            audioDeviceName = std::string(wic.szPname);
            std::cout << "[DEBUG] Audio: Using device index " << captureDeviceIndex << ": " << audioDeviceName << std::endl;
            std::cout << "[DEBUG] Audio: Device supports " << (int)wic.wChannels << " channels" << std::endl;
            
            logAudio("Using device: " + audioDeviceName);
            logAudio("Device supports " + std::to_string((int)wic.wChannels) + " channels");
            logAudio("Sample rate: " + std::to_string(sampleRate) + " Hz");
        }
    } else {
        logAudio("Audio capture initialized - No audio devices found");
        return false;
    }
    
    MMRESULT result = waveInOpen(&hWaveIn, captureDeviceIndex, &wfx, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
    std::cout << "[DEBUG] Audio: waveInOpen result: " << result << std::endl;
    if (result != MMSYSERR_NOERROR) {
        std::cerr << "[ERROR] Audio: waveInOpen failed: " << result << std::endl;
        return false;
    }
    
    for (int i = 0; i < 2; i++) {
        waveHdr[i].lpData = (LPSTR)malloc(captureSamplesPerBuffer * sizeof(short));
        waveHdr[i].dwBufferLength = captureSamplesPerBuffer * sizeof(short);
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
    
    {
        std::lock_guard<std::mutex> lock(audioMutex);
        capturedSamples.clear();
    }
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
    std::cout << "[DEBUG] Audio: waveInStart result: " << result << std::endl;
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
    
    // Prevent callback from re-queuing buffers while we stop/reset
    audioCapturing = false;

    std::cout << "[DEBUG] Audio: waveInStop start" << std::endl;
    MMRESULT stopResult = waveInStop(hWaveIn);
    std::cout << "[DEBUG] Audio: waveInStop done (" << stopResult << ")" << std::endl;

    std::cout << "[DEBUG] Audio: waveInReset start" << std::endl;
    MMRESULT resetResult = waveInReset(hWaveIn);
    std::cout << "[DEBUG] Audio: waveInReset done (" << resetResult << ")" << std::endl;

    for (int i = 0; i < 2; i++) {
        std::cout << "[DEBUG] Audio: waveInUnprepareHeader start [" << i << "]" << std::endl;
        waveInUnprepareHeader(hWaveIn, &waveHdr[i], sizeof(WAVEHDR));
        std::cout << "[DEBUG] Audio: waveInUnprepareHeader done [" << i << "]" << std::endl;
    }
    
    std::cout << "[DEBUG] Audio: Capture stopped" << std::endl;
}

bool isAudioCapturing() {
    return audioCapturing;
}

std::vector<short> getCapturedAudioSamples() {
    std::lock_guard<std::mutex> lock(audioMutex);
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

int getAudioCaptureDeviceIndex() {
    return 0;
}

bool setAudioCaptureDeviceIndex(int index) {
    (void)index;
    return false;
}

int getAudioCaptureCallbackCount() {
    return 0;
}

int getAudioCaptureZeroByteCount() {
    return 0;
}
#endif
