#ifndef AUDIOSERVICE_H
#define AUDIOSERVICE_H

#include "IAudioService.h"

/**
 * AudioService - Audio service implementation
 * 
 * Manages audio generation, capture, and network subsystems
 * Non-fatal service (continues even if audio initialization fails)
 */
class AudioService : public IAudioService {
public:
    AudioService();
    ~AudioService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;

private:
    bool initialized_;
};

#endif // AUDIOSERVICE_H
