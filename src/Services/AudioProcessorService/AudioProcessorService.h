#ifndef AUDIOPROCESSORSERVICE_H
#define AUDIOPROCESSORSERVICE_H

#include "IAudioProcessorService.h"
#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

class IAudioCaptureService;
class ISTTService;

/**
 * AudioProcessorService - Speech chunking and silence detection
 */
class AudioProcessorService : public IAudioProcessorService {
public:
    AudioProcessorService();
    ~AudioProcessorService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;

    bool PopSpeechChunk(std::vector<float>& outSamples) override;
    bool IsSilent() const;

    static AudioProcessorService* GetInstance();

private:
    void RunWorker();
    void AppendSamples(std::vector<float>& buffer, const float* samples, int count);

    static AudioProcessorService* instance_;

    mutable std::mutex mutex_;
    std::deque<std::vector<float>> readyChunks_;
    std::vector<float> currentChunk_;
    std::vector<float> tailBuffer_;
    std::vector<float> preSpeechBuffer_;
    bool speaking_;
    int silenceMs_;
    int speechMs_;
    std::atomic<bool> silentFlag_;
    std::atomic<bool> running_;
    std::thread worker_;
    IAudioCaptureService* capture_;
    ISTTService* stt_;
};

#endif // AUDIOPROCESSORSERVICE_H
