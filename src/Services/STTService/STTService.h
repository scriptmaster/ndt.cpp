#ifndef STTSERVICE_H
#define STTSERVICE_H

#include "ISTTService.h"
#include "safety/foreign_pointer.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/**
 * STTService - Speech-to-Text service implementation
 * 
 * Realtime Whisper STT
 */
struct whisper_context;

// Custom deleter for whisper_context (RAII)
struct WhisperContextDeleter {
    void operator()(whisper_context* ctx) const noexcept;
};

class STTService : public ISTTService {
public:
    STTService();
    ~STTService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;

    std::string Transcribe(const std::vector<float>& samples) override;
    std::string Transcribe(const float* samples, int count) override;
    std::string Transcribe(const int16_t* samples, int count) override;
    std::string TranscribeBlocking(const int16_t* samples, int count);
    static STTService* GetInstance();

private:
    static STTService* instance_;
    safety::ForeignPointer<whisper_context*, WhisperContextDeleter> ctx_;
    std::string modelPath_;
    std::mutex ctxMutex_;

    std::mutex queueMutex_;
    std::condition_variable queueCv_;
    std::deque<std::vector<int16_t>> queue_;
    std::atomic<bool> running_;
    std::atomic<bool> workerFailed_;
    std::atomic<bool> available_;
    std::atomic<bool> loggedUnavailable_;
    std::mutex workerMutex_;
    std::thread worker_;

    bool EnsureContextLocked();
    bool EnsureWorkerStarted();
    void EnqueuePcm(const int16_t* samples, int count);
    void WorkerLoop();
};

#endif // STTSERVICE_H
