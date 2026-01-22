#include "STTService.h"
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "ggml-backend.h"
#include "whisper.h"

/**
 * STTService implementation
 * Whisper-backed transcription
 */

STTService* STTService::instance_ = nullptr;

namespace {
constexpr std::uintmax_t kMinModelSizeBytes = 5ULL * 1024 * 1024;

std::uintmax_t getFileSizeIfExists(const std::string& path) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return 0;
    }
    return fs::file_size(path, ec);
}

std::string resolveModelPath() {
    namespace fs = std::filesystem;
    const std::vector<std::string> modelFiles = {
        "ggml-base.en.bin",
        "ggml-base.bin",
    };

    std::vector<std::string> candidateDirs = {
        "models/whisper",
        "models",
        "whisper",
        ".",
        "../models/whisper",
        "../models",
        "../whisper",
        "..",
        "../../models/whisper",
        "../../models",
        "../../whisper",
        "../..",
    };

    const char* userProfile = std::getenv("USERPROFILE");
    const char* home = std::getenv("HOME");
    std::string userHome;
    if (userProfile && *userProfile) {
        userHome = userProfile;
    } else if (home && *home) {
        userHome = home;
    }

    if (!userHome.empty()) {
        candidateDirs.push_back(userHome + "/models");
        candidateDirs.push_back(userHome + "/.cache");
        candidateDirs.push_back(userHome + "/.cache/models");
        candidateDirs.push_back(userHome + "/.cache/whisper/models");
        candidateDirs.push_back(userHome + "/.cache/whisper.cpp/models");
    }

    for (const auto& dir : candidateDirs) {
        for (const auto& file : modelFiles) {
            fs::path candidate = fs::path(dir) / file;
            std::error_code ec;
            if (fs::exists(candidate, ec)) {
                return candidate.string();
            }
        }
    }
    return "";
}
} // namespace

STTService::STTService() {
    ctx_ = nullptr;
    running_.store(false);
    workerFailed_.store(false);
    available_.store(true);
    loggedUnavailable_.store(false);
    instance_ = this;
}

STTService::~STTService() {
    Stop();
    instance_ = nullptr;
}

void STTService::Configure() {
    modelPath_.clear();
}

bool STTService::EnsureContextLocked() {
    if (modelPath_.empty()) {
        Configure();
    }
    if (modelPath_.empty()) {
        modelPath_ = resolveModelPath();
    }
    if (!ctx_) {
        if (modelPath_.empty()) {
            available_.store(false);
            if (!loggedUnavailable_.exchange(true)) {
                std::cerr << "[ERROR] STT: Whisper model not found. STT unavailable." << std::endl;
            }
            return false;
        }

        std::uintmax_t sizeBytes = getFileSizeIfExists(modelPath_);
        if (sizeBytes < kMinModelSizeBytes) {
            available_.store(false);
            if (!loggedUnavailable_.exchange(true)) {
                std::cerr << "[ERROR] STT: Whisper model invalid (" << sizeBytes
                          << " bytes): " << modelPath_ << ". STT unavailable." << std::endl;
            }
            return false;
        }

        std::cout << "[DEBUG] STT: Loading Whisper model from " << modelPath_ << std::endl;
        whisper_context_params cparams = whisper_context_default_params();
        cparams.use_gpu = false;
        cparams.flash_attn = false;
        ctx_ = whisper_init_from_file_with_params(modelPath_.c_str(), cparams);
        if (!ctx_) {
            available_.store(false);
            if (!loggedUnavailable_.exchange(true)) {
                std::cerr << "[ERROR] STT: Failed to load Whisper model: " << modelPath_
                          << ". STT unavailable." << std::endl;
            }
            return false;
        }
        available_.store(true);
        loggedUnavailable_.store(false);
        std::cout << "[DEBUG] STT: Whisper model loaded: " << modelPath_ << std::endl;
    }
    return true;
}

bool STTService::Start() {
    running_.store(true);
    workerFailed_.store(false);
    available_.store(true);
    return true;
}

void STTService::Stop() {
    running_.store(false);
    queueCv_.notify_all();
    {
        std::lock_guard<std::mutex> lock(workerMutex_);
        if (worker_.joinable()) {
            worker_.join();
        }
    }
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        queue_.clear();
    }
    std::lock_guard<std::mutex> lock(ctxMutex_);
    if (ctx_) {
        whisper_free(ctx_);
        ctx_ = nullptr;
    }
    available_.store(false);
    loggedUnavailable_.store(false);
}

STTService* STTService::GetInstance() {
    return instance_;
}

std::string STTService::Transcribe(const std::vector<float>& samples) {
    return Transcribe(samples.data(), static_cast<int>(samples.size()));
}

std::string STTService::Transcribe(const float* samples, int count) {
    if (!samples || count <= 0) {
        return "";
    }
    std::vector<int16_t> pcm;
    pcm.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; i++) {
        float s = samples[i];
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        pcm.push_back(static_cast<int16_t>(std::lround(s * 32768.0f)));
    }
    return Transcribe(pcm.data(), static_cast<int>(pcm.size()));
}

std::string STTService::Transcribe(const int16_t* samples, int count) {
    if (!samples || count <= 0) {
        return "";
    }

    EnqueuePcm(samples, count);
    return "";
}

std::string STTService::TranscribeBlocking(const int16_t* samples, int count) {
    if (!samples || count <= 0) {
        return "";
    }

    try {
    std::vector<float> floatSamples;
    floatSamples.reserve(static_cast<size_t>(count));
    float peak = 0.0f;
    for (int i = 0; i < count; i++) {
        float v = static_cast<float>(samples[i]) / 32768.0f;
        floatSamples.push_back(v);
        float absV = std::fabs(v);
        if (absV > peak) {
            peak = absV;
        }
    }

    std::lock_guard<std::mutex> lock(ctxMutex_);
    if (!EnsureContextLocked()) {
        std::cerr << "[ERROR] STT: Context unavailable; cannot transcribe." << std::endl;
        return "";
    }
    if (ggml_backend_dev_count() == 0) {
        available_.store(false);
        if (!loggedUnavailable_.exchange(true)) {
            std::cerr << "[ERROR] STT: No GGML backend devices available; STT unavailable." << std::endl;
        }
        return "";
    }

    std::cout << "[DEBUG] STT: Transcribe start | samples=" << floatSamples.size()
              << " | sample_rate=16000 | peak=" << peak
              << " | thread=" << std::this_thread::get_id()
              << " | ctx=" << ctx_ << std::endl;

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.print_realtime = false;
    params.print_timestamps = false;
    params.language = "en";

    std::cout << "[DEBUG] STT: whisper_full begin | threads=" << params.n_threads
              << " | language=" << (params.language ? params.language : "null")
              << " | samples=" << static_cast<int>(floatSamples.size()) << std::endl;
    int whisperResult = whisper_full(ctx_, params, floatSamples.data(), static_cast<int>(floatSamples.size()));
    if (whisperResult != 0) {
        std::cerr << "[ERROR] STT: whisper_full failed" << std::endl;
        return "";
    }
    std::cout << "[DEBUG] STT: whisper_full complete | result=" << whisperResult << std::endl;

    std::string transcript;
    int segments = whisper_full_n_segments(ctx_);
    std::cout << "[DEBUG] STT: segments=" << segments << std::endl;
    for (int i = 0; i < segments; i++) {
        const char* text = whisper_full_get_segment_text(ctx_, i);
        if (text) {
            transcript += text;
        }
    }

    if (!transcript.empty()) {
        std::cout << "[STT] " << transcript << std::endl;
    }

    return transcript;
    } catch (const std::exception& e) {
        available_.store(false);
        if (!loggedUnavailable_.exchange(true)) {
            std::cerr << "[ERROR] STT: Exception during transcription: " << e.what() << std::endl;
        }
        return "";
    } catch (...) {
        available_.store(false);
        if (!loggedUnavailable_.exchange(true)) {
            std::cerr << "[ERROR] STT: Unknown exception during transcription" << std::endl;
        }
        return "";
    }
}

bool STTService::EnsureWorkerStarted() {
    if (!running_.load() || workerFailed_.load()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(workerMutex_);
    if (worker_.joinable()) {
        return true;
    }
    if (!running_.load()) {
        return false;
    }
    try {
        worker_ = std::thread(&STTService::WorkerLoop, this);
        return true;
    } catch (const std::exception& e) {
        workerFailed_.store(true);
        running_.store(false);
        std::cerr << "[ERROR] STT: Failed to start worker thread: " << e.what() << std::endl;
        return false;
    } catch (...) {
        workerFailed_.store(true);
        running_.store(false);
        std::cerr << "[ERROR] STT: Failed to start worker thread" << std::endl;
        return false;
    }
}

void STTService::EnqueuePcm(const int16_t* samples, int count) {
    if (!samples || count <= 0) {
        return;
    }
    if (!available_.load()) {
        if (!loggedUnavailable_.exchange(true)) {
            std::cerr << "[ERROR] STT: Service unavailable; dropping audio segment" << std::endl;
        }
        return;
    }
    if (!EnsureWorkerStarted()) {
        std::cerr << "[ERROR] STT: Worker not available; dropping segment" << std::endl;
        return;
    }
    std::vector<int16_t> pcm(samples, samples + count);
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        queue_.push_back(std::move(pcm));
    }
    queueCv_.notify_one();
}

void STTService::WorkerLoop() {
    std::cout << "[DEBUG] STT: Worker started | thread=" << std::this_thread::get_id() << std::endl;
    while (running_.load()) {
        std::vector<int16_t> pcm;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCv_.wait(lock, [&]() { return !running_.load() || !queue_.empty(); });
            if (!running_.load()) {
                break;
            }
            pcm = std::move(queue_.front());
            queue_.pop_front();
        }

        if (pcm.empty()) {
            continue;
        }

        std::vector<float> floatSamples;
        floatSamples.reserve(pcm.size());
        float peak = 0.0f;
        for (int16_t sample : pcm) {
            float v = static_cast<float>(sample) / 32768.0f;
            floatSamples.push_back(v);
            float absV = std::fabs(v);
            if (absV > peak) {
                peak = absV;
            }
        }

        try {
            std::lock_guard<std::mutex> lock(ctxMutex_);
            if (!EnsureContextLocked()) {
                if (!loggedUnavailable_.exchange(true)) {
                    std::cerr << "[ERROR] STT: Service unavailable; dropping segment" << std::endl;
                }
                continue;
            }
            if (ggml_backend_dev_count() == 0) {
                available_.store(false);
                if (!loggedUnavailable_.exchange(true)) {
                    std::cerr << "[ERROR] STT: No GGML backend devices available; STT unavailable." << std::endl;
                }
                continue;
            }

            std::cout << "[DEBUG] STT: Transcribe start | samples=" << floatSamples.size()
                      << " | sample_rate=16000 | peak=" << peak
                      << " | thread=" << std::this_thread::get_id()
                      << " | ctx=" << ctx_ << std::endl;

            whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
            params.print_progress = false;
            params.print_realtime = false;
            params.print_timestamps = false;
            params.language = "en";

            std::cout << "[DEBUG] STT: whisper_full begin | threads=" << params.n_threads
                      << " | language=" << (params.language ? params.language : "null")
                      << " | samples=" << static_cast<int>(floatSamples.size()) << std::endl;
            int whisperResult = whisper_full(ctx_, params, floatSamples.data(), static_cast<int>(floatSamples.size()));
            if (whisperResult != 0) {
                std::cerr << "[ERROR] STT: whisper_full failed" << std::endl;
                continue;
            }
            std::cout << "[DEBUG] STT: whisper_full complete | result=" << whisperResult << std::endl;

            std::string transcript;
            int segments = whisper_full_n_segments(ctx_);
            std::cout << "[DEBUG] STT: segments=" << segments << std::endl;
            for (int i = 0; i < segments; i++) {
                const char* text = whisper_full_get_segment_text(ctx_, i);
                if (text) {
                    transcript += text;
                }
            }

            if (!transcript.empty()) {
                std::cout << "[STT] " << transcript << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] STT: Exception during transcription: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] STT: Unknown exception during transcription" << std::endl;
        }
    }
    std::cout << "[DEBUG] STT: Worker stopped" << std::endl;
}
