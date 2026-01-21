#ifndef AUDIOCAPTURESERVICE_H
#define AUDIOCAPTURESERVICE_H

#include "IAudioCaptureService.h"
#include <cstddef>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>

/**
 * AudioCaptureService - Audio capture lifecycle service
 */
class AudioCaptureService : public IAudioCaptureService {
public:
    AudioCaptureService();
    ~AudioCaptureService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;

    bool DequeueAudio(float* out, int maxSamples) override;
    void EnqueueAudioSamples(const float* samples, int count);
    static AudioCaptureService* GetInstance();

private:
    bool initialized_;
    bool testMode_;
    std::thread feederThread_;
    std::atomic<bool> feederRunning_;
    std::mutex audioMutex_;
    std::vector<float> ringBuffer_;
    size_t ringCapacity_;
    size_t readIndex_;
    size_t writeIndex_;
    size_t available_;
    static AudioCaptureService* instance_;
};

#endif // AUDIOCAPTURESERVICE_H
