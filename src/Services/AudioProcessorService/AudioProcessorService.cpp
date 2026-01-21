#include "AudioProcessorService.h"
#include "../AudioCaptureService/AudioCaptureService.h"
#include "../STTService/STTService.h"
#include <cmath>
#include <cstdint>
#include <chrono>
#include <iostream>
#include <thread>

AudioProcessorService* AudioProcessorService::instance_ = nullptr;

namespace {
constexpr float kSilenceRms = 0.0035f;
constexpr int kBufferMs = 20;
constexpr int kEndSilenceMs = 600;
constexpr int kTailMs = 200;
constexpr int kSampleRate = 44100;
constexpr int kWhisperSampleRate = 16000;
constexpr int kMaxChunkSeconds = 10;
constexpr int kMinChunkMs = 300;
}

AudioProcessorService::AudioProcessorService()
    : speaking_(false),
      silenceMs_(0),
      speechMs_(0),
      silentFlag_(true),
      running_(false),
      capture_(nullptr),
      stt_(nullptr) {
    currentChunk_.reserve(kSampleRate * kMaxChunkSeconds);
    tailBuffer_.reserve((kSampleRate * kTailMs) / 1000);
    preSpeechBuffer_.reserve((kSampleRate * 2) / 10);
    instance_ = this;
}

AudioProcessorService::~AudioProcessorService() {
    instance_ = nullptr;
}

void AudioProcessorService::Configure() {
}

bool AudioProcessorService::Start() {
    if (running_.load()) {
        return true;
    }
    capture_ = AudioCaptureService::GetInstance();
    stt_ = STTService::GetInstance();
    if (!capture_) {
        std::cerr << "[ERROR] AudioProcessorService: AudioCaptureService not available" << std::endl;
    }
    if (!stt_) {
        std::cerr << "[ERROR] AudioProcessorService: STTService not available" << std::endl;
    }
    running_.store(true);
    worker_ = std::thread(&AudioProcessorService::RunWorker, this);
    return true;
}

void AudioProcessorService::Stop() {
    running_.store(false);
    if (worker_.joinable()) {
        worker_.join();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    readyChunks_.clear();
    currentChunk_.clear();
    tailBuffer_.clear();
    preSpeechBuffer_.clear();
    speaking_ = false;
    silenceMs_ = 0;
    speechMs_ = 0;
    silentFlag_.store(true, std::memory_order_relaxed);
}

AudioProcessorService* AudioProcessorService::GetInstance() {
    return instance_;
}

bool AudioProcessorService::IsSilent() const {
    return silentFlag_.load(std::memory_order_relaxed);
}

void AudioProcessorService::AppendSamples(std::vector<float>& buffer, const float* samples, int count) {
    if (count <= 0 || !samples) {
        return;
    }
    if (buffer.size() + static_cast<size_t>(count) > buffer.capacity()) {
        int available = static_cast<int>(buffer.capacity() - buffer.size());
        if (available <= 0) {
            return;
        }
        count = available;
    }
    buffer.insert(buffer.end(), samples, samples + count);
}

void AudioProcessorService::RunWorker() {
    std::vector<float> buffer((kSampleRate * kBufferMs) / 1000);
    const int bufferSamples = static_cast<int>(buffer.size());
    const int startMs = 60;

    while (running_.load()) {
        if (!capture_ || !capture_->DequeueAudio(buffer.data(), bufferSamples)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        float rmsAccumulator = 0.0f;
        for (int i = 0; i < bufferSamples; i++) {
            float s = buffer[i];
            rmsAccumulator += s * s;
        }
        float rms = std::sqrt(rmsAccumulator / bufferSamples);
        bool bufferSilent = rms < kSilenceRms;

        std::vector<float> completedChunk;
        bool shouldTranscribe = false;

        {
            std::lock_guard<std::mutex> lock(mutex_);

        if (!speaking_) {
            if (bufferSilent) {
                speechMs_ = 0;
                preSpeechBuffer_.clear();
                silentFlag_.store(true, std::memory_order_relaxed);
                continue;
            }
            speechMs_ += kBufferMs;
            AppendSamples(preSpeechBuffer_, buffer.data(), bufferSamples);
            if (speechMs_ >= startMs) {
                speaking_ = true;
                silenceMs_ = 0;
                tailBuffer_.clear();
                currentChunk_.clear();
                AppendSamples(currentChunk_, preSpeechBuffer_.data(), static_cast<int>(preSpeechBuffer_.size()));
                preSpeechBuffer_.clear();
                std::cout << "Speech started" << std::endl;
            }
            silentFlag_.store(false, std::memory_order_relaxed);
            continue;
        }

        if (!bufferSilent) {
            silenceMs_ = 0;
            tailBuffer_.clear();
            AppendSamples(currentChunk_, buffer.data(), bufferSamples);
            silentFlag_.store(false, std::memory_order_relaxed);
            continue;
        }

        silenceMs_ += kBufferMs;
        AppendSamples(tailBuffer_, buffer.data(), bufferSamples);
        if (tailBuffer_.size() > (kSampleRate * kTailMs) / 1000) {
            size_t excess = tailBuffer_.size() - (kSampleRate * kTailMs) / 1000;
            tailBuffer_.erase(tailBuffer_.begin(), tailBuffer_.begin() + excess);
        }

        if (silenceMs_ >= kEndSilenceMs) {
            AppendSamples(currentChunk_, tailBuffer_.data(), static_cast<int>(tailBuffer_.size()));
            const int samplesCount = static_cast<int>(currentChunk_.size());
            const int minSamples = (kSampleRate * kMinChunkMs) / 1000;
            const int maxSamples = kSampleRate * kMaxChunkSeconds;
            std::cout << "Speech ended, samples=" << samplesCount << std::endl;
            if (samplesCount < minSamples || samplesCount > maxSamples) {
                std::cout << "[AudioProcessor] segment dropped (too "
                          << (samplesCount < minSamples ? "short" : "long") << ")" << std::endl;
                currentChunk_.clear();
                currentChunk_.reserve(kSampleRate * kMaxChunkSeconds);
            } else if (!currentChunk_.empty()) {
                completedChunk = std::move(currentChunk_);
                currentChunk_.clear();
                currentChunk_.reserve(kSampleRate * kMaxChunkSeconds);
                shouldTranscribe = true;
            }
            tailBuffer_.clear();
            speaking_ = false;
            silenceMs_ = 0;
            speechMs_ = 0;
            silentFlag_.store(true, std::memory_order_relaxed);
        } else {
            silentFlag_.store(false, std::memory_order_relaxed);
        }

        }

        if (shouldTranscribe && stt_ && !completedChunk.empty()) {
            std::vector<float> resampled;
            const double resampleRatio = static_cast<double>(kWhisperSampleRate) / kSampleRate;
            const int outCount = static_cast<int>(std::round(completedChunk.size() * resampleRatio));
            if (outCount > 0) {
                resampled.resize(static_cast<size_t>(outCount));
                const double step = static_cast<double>(kSampleRate) / kWhisperSampleRate;
                for (int i = 0; i < outCount; i++) {
                    double srcIndex = i * step;
                    int idx = static_cast<int>(srcIndex);
                    double frac = srcIndex - idx;
                    if (idx < 0) {
                        resampled[static_cast<size_t>(i)] = completedChunk.front();
                    } else if (idx >= static_cast<int>(completedChunk.size()) - 1) {
                        resampled[static_cast<size_t>(i)] = completedChunk.back();
                    } else {
                        float a = completedChunk[static_cast<size_t>(idx)];
                        float b = completedChunk[static_cast<size_t>(idx + 1)];
                        resampled[static_cast<size_t>(i)] = static_cast<float>((1.0 - frac) * a + frac * b);
                    }
                }
            }

            const std::vector<float>& toConvert = resampled.empty() ? completedChunk : resampled;
            std::vector<int16_t> pcm;
            pcm.reserve(toConvert.size());
            for (float s : toConvert) {
                if (s > 1.0f) s = 1.0f;
                if (s < -1.0f) s = -1.0f;
                pcm.push_back(static_cast<int16_t>(std::lround(s * 32768.0f)));
            }
            if (!pcm.empty()) {
                std::cout << "[DEBUG] STT: dispatch segment | samples=" << pcm.size()
                          << " | thread=" << std::this_thread::get_id() << std::endl;
                stt_->Transcribe(pcm.data(), static_cast<int>(pcm.size()));
            }
        }
    }
}

bool AudioProcessorService::PopSpeechChunk(std::vector<float>& outSamples) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (readyChunks_.empty()) {
        return false;
    }
    outSamples = std::move(readyChunks_.front());
    readyChunks_.pop_front();
    return true;
}
