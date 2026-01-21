#include "STTService.h"
#include "../DownloaderService/DownloaderService.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "whisper.h"

/**
 * STTService implementation
 * Whisper-backed transcription
 */

STTService* STTService::instance_ = nullptr;

namespace {
const char* kModelUrl = "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin";
const char* kLocalModelPath = "models/whisper/ggml-base.en.bin";

std::string resolveModelPath() {
    namespace fs = std::filesystem;

    std::vector<std::string> candidates = {
        "models/whisper/ggml-base.en.bin",
        "models/ggml-base.en.bin",
        "whisper/ggml-base.en.bin",
        "ggml-base.en.bin",
        "../models/whisper/ggml-base.en.bin",
        "../models/ggml-base.en.bin",
        "../whisper/ggml-base.en.bin",
        "../ggml-base.en.bin",
        "../../models/whisper/ggml-base.en.bin",
        "../../models/ggml-base.en.bin",
        "../../whisper/ggml-base.en.bin",
        "../../ggml-base.en.bin",
    };

    for (const auto& path : candidates) {
        std::error_code ec;
        if (fs::exists(path, ec)) {
            return path;
        }
    }
    return "";
}
} // namespace

STTService::STTService() {
    ctx_ = nullptr;
    instance_ = this;
}

STTService::~STTService() {
    Stop();
    instance_ = nullptr;
}

void STTService::Configure() {
    modelPath_.clear();
}

bool STTService::Start() {
    if (modelPath_.empty()) {
        Configure();
    }
    if (modelPath_.empty()) {
        modelPath_ = resolveModelPath();
    }
    if (modelPath_.empty()) {
        std::string downloaded = DownloaderService::downloadHFModel(kModelUrl, kLocalModelPath);
        if (!downloaded.empty()) {
            modelPath_ = downloaded;
        }
    }
    if (modelPath_.empty()) {
        modelPath_ = resolveModelPath();
    }
    if (!ctx_) {
        if (modelPath_.empty()) {
            std::cerr << "[ERROR] STT: Whisper model not found" << std::endl;
            return false;
        }
        std::cout << "[DEBUG] STT: Loading Whisper model from " << modelPath_ << std::endl;
        ctx_ = whisper_init_from_file(modelPath_.c_str());
        if (!ctx_) {
            std::cerr << "[ERROR] STT: Failed to load Whisper model: " << modelPath_ << std::endl;
            return false;
        }
        std::cout << "[DEBUG] STT: Whisper model loaded: " << modelPath_ << std::endl;
    }
    return true;
}

void STTService::Stop() {
    if (ctx_) {
        whisper_free(ctx_);
        ctx_ = nullptr;
    }
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
    if (!ctx_) {
        std::cerr << "[ERROR] STT: Whisper context not initialized; attempting Start()" << std::endl;
        if (!Start() || !ctx_) {
            std::cerr << "[ERROR] STT: Start() failed; aborting transcription" << std::endl;
            return "";
        }
    }

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

    std::cout << "[DEBUG] STT: Transcribe start | samples=" << count
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
}
