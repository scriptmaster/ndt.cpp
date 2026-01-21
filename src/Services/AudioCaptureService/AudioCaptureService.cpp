#include "AudioCaptureService.h"
#include "AudioCapture.h"
#include "../LoggingService/SceneLogger.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

using namespace std::chrono_literals;

static bool runWithTimeout(const std::function<void()>& work, std::chrono::milliseconds timeout) {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;

    std::thread worker([&]() {
        try {
            work();
        } catch (...) {
            // Ensure we still signal completion
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
        }
        cv.notify_one();
    });

    std::unique_lock<std::mutex> lock(mutex);
    if (!cv.wait_for(lock, timeout, [&]() { return done; })) {
        worker.detach();
        return false;
    }

    worker.join();
    return true;
}

AudioCaptureService* AudioCaptureService::instance_ = nullptr;

AudioCaptureService::AudioCaptureService()
    : initialized_(false),
      testMode_(false),
      feederRunning_(false),
      ringCapacity_(0),
      readIndex_(0),
      writeIndex_(0),
      available_(0) {
    instance_ = this;
}

AudioCaptureService::~AudioCaptureService() {
    instance_ = nullptr;
}

void AudioCaptureService::Configure() {
    // No configuration needed
}

bool AudioCaptureService::Start() {
    std::cout << "[DEBUG] Initializing audio capture..." << std::endl;
    try {
        initAudioLogger();
        const char* envMode = std::getenv("ENV");
        testMode_ = envMode && std::string(envMode) == "test";
        ringCapacity_ = 44100 * 5;
        ringBuffer_.assign(ringCapacity_, 0.0f);
        readIndex_ = 0;
        writeIndex_ = 0;
        available_ = 0;

        if (testMode_) {
            const char* wavPathEnv = std::getenv("TEST_WAV");
            std::string wavPath = wavPathEnv ? wavPathEnv : "test.wav";

            std::ifstream file(wavPath, std::ios::binary);
            if (!file) {
                std::cerr << "[ERROR] AudioCaptureService test mode: failed to open " << wavPath << std::endl;
                cleanupAudioLogger();
                return true;
            }

            auto readU32 = [&](uint32_t& out) { file.read(reinterpret_cast<char*>(&out), sizeof(out)); };
            auto readU16 = [&](uint16_t& out) { file.read(reinterpret_cast<char*>(&out), sizeof(out)); };

            char riff[4] = {};
            file.read(riff, 4);
            uint32_t riffSize = 0;
            readU32(riffSize);
            char wave[4] = {};
            file.read(wave, 4);

            if (std::strncmp(riff, "RIFF", 4) != 0 || std::strncmp(wave, "WAVE", 4) != 0) {
                std::cerr << "[ERROR] AudioCaptureService test mode: invalid WAV header" << std::endl;
                cleanupAudioLogger();
                return true;
            }

            uint16_t audioFormat = 0;
            uint16_t numChannels = 0;
            uint32_t sampleRate = 0;
            uint16_t bitsPerSample = 0;
            uint32_t dataSize = 0;
            std::streampos dataPos = 0;

            while (file && !dataPos) {
                char chunkId[4] = {};
                uint32_t chunkSize = 0;
                file.read(chunkId, 4);
                readU32(chunkSize);
                if (std::strncmp(chunkId, "fmt ", 4) == 0) {
                    readU16(audioFormat);
                    readU16(numChannels);
                    readU32(sampleRate);
                    uint32_t byteRate = 0;
                    uint16_t blockAlign = 0;
                    readU32(byteRate);
                    readU16(blockAlign);
                    readU16(bitsPerSample);
                    if (chunkSize > 16) {
                        file.seekg(chunkSize - 16, std::ios::cur);
                    }
                } else if (std::strncmp(chunkId, "data", 4) == 0) {
                    dataSize = chunkSize;
                    dataPos = file.tellg();
                    file.seekg(chunkSize, std::ios::cur);
                } else {
                    file.seekg(chunkSize, std::ios::cur);
                }
            }

            if (!dataPos || audioFormat != 1 || bitsPerSample != 16 || sampleRate == 0) {
                std::cerr << "[ERROR] AudioCaptureService test mode: unsupported WAV format" << std::endl;
                cleanupAudioLogger();
                return true;
            }

            if (sampleRate != 44100) {
                std::cerr << "[WARNING] AudioCaptureService test mode: expected 44100Hz, got " << sampleRate << std::endl;
            }

            file.clear();
            file.seekg(dataPos);
            std::vector<int16_t> pcm(dataSize / sizeof(int16_t));
            file.read(reinterpret_cast<char*>(pcm.data()), dataSize);

            std::vector<float> mono;
            mono.reserve(pcm.size() / numChannels);
            for (size_t i = 0; i + (numChannels - 1) < pcm.size(); i += numChannels) {
                int32_t sum = 0;
                for (int ch = 0; ch < numChannels; ch++) {
                    sum += pcm[i + ch];
                }
                int16_t m = static_cast<int16_t>(sum / static_cast<int32_t>(numChannels));
                mono.push_back(static_cast<float>(m) / 32768.0f);
            }

            feederRunning_.store(true);
            feederThread_ = std::thread([this, mono = std::move(mono), sampleRate]() mutable {
                const int chunkSamples = static_cast<int>((sampleRate * 20) / 1000);
                size_t offset = 0;
                while (feederRunning_.load()) {
                    if (offset < mono.size()) {
                        int count = std::min(static_cast<size_t>(chunkSamples), mono.size() - offset);
                        EnqueueAudioSamples(mono.data() + offset, count);
                        offset += count;
                    } else {
                        std::vector<float> silence(chunkSamples, 0.0f);
                        EnqueueAudioSamples(silence.data(), chunkSamples);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
            });

            initialized_ = true;
            std::cout << "[DEBUG] AudioCaptureService test mode: feeding " << wavPath << std::endl;
            return true;
        }

        if (!initAudioCapture(44100)) {
            std::cerr << "[WARNING] Audio capture initialization failed - STT will not receive audio" << std::endl;
            cleanupAudioLogger();
            return true;
        }
        startAudioCapture();
        initialized_ = true;
        std::cout << "[DEBUG] Audio capture initialized - SUCCESS" << std::endl;
        return true;
    } catch (...) {
        std::cerr << "[ERROR] Audio capture initialization failed with exception" << std::endl;
        return true; // Non-fatal
    }
}

void AudioCaptureService::Stop() {
    if (!initialized_) {
        return;
    }

    std::cout << "[DEBUG] Stopping audio capture with timeout guard..." << std::endl;
    if (testMode_) {
        feederRunning_.store(false);
        if (feederThread_.joinable()) {
            feederThread_.join();
        }
    }
    bool finished = runWithTimeout([]() {
        std::cout << "[DEBUG] AudioCaptureService::Stop - stopAudioCapture start" << std::endl;
        stopAudioCapture();
        std::cout << "[DEBUG] AudioCaptureService::Stop - stopAudioCapture done" << std::endl;

        std::cout << "[DEBUG] AudioCaptureService::Stop - cleanupAudioCapture start" << std::endl;
        cleanupAudioCapture();
        std::cout << "[DEBUG] AudioCaptureService::Stop - cleanupAudioCapture done" << std::endl;
    }, 3s);

    if (!finished) {
        std::cerr << "[ERROR] Audio capture cleanup timed out; continuing shutdown" << std::endl;
    } else {
        std::cout << "[DEBUG] Audio capture stopped - SUCCESS" << std::endl;
    }

    {
        std::lock_guard<std::mutex> lock(audioMutex_);
        ringBuffer_.clear();
        ringCapacity_ = 0;
        readIndex_ = 0;
        writeIndex_ = 0;
        available_ = 0;
    }
    cleanupAudioLogger();
    initialized_ = false;
}

AudioCaptureService* AudioCaptureService::GetInstance() {
    return instance_;
}

void AudioCaptureService::EnqueueAudioSamples(const float* samples, int count) {
    if (!samples || count <= 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(audioMutex_);
    if (ringCapacity_ == 0) {
        return;
    }
    for (int i = 0; i < count; i++) {
        if (available_ == ringCapacity_) {
            readIndex_ = (readIndex_ + 1) % ringCapacity_;
            available_--;
        }
        ringBuffer_[writeIndex_] = samples[i];
        writeIndex_ = (writeIndex_ + 1) % ringCapacity_;
        available_++;
    }
}

bool AudioCaptureService::DequeueAudio(float* out, int maxSamples) {
    if (!out || maxSamples <= 0) {
        return false;
    }
    std::lock_guard<std::mutex> lock(audioMutex_);
    if (available_ < static_cast<size_t>(maxSamples)) {
        return false;
    }
    for (int i = 0; i < maxSamples; i++) {
        out[i] = ringBuffer_[readIndex_];
        readIndex_ = (readIndex_ + 1) % ringCapacity_;
        available_--;
    }
    return true;
}
