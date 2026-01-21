#ifndef AUDIOPLAYERSERVICE_H
#define AUDIOPLAYERSERVICE_H

#include "IAudioPlayerService.h"

/**
 * AudioPlayerService - Audio playback/generation service
 */
class AudioPlayerService : public IAudioPlayerService {
public:
    AudioPlayerService();
    ~AudioPlayerService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;

private:
    bool initialized_;
};

#endif // AUDIOPLAYERSERVICE_H
