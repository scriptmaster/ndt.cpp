#ifndef IAUDIOPLAYERSERVICE_H
#define IAUDIOPLAYERSERVICE_H

#include "../../App/DI/IService.h"

/**
 * IAudioPlayerService - Audio playback/generation service interface
 */
class IAudioPlayerService : public IService {
public:
    virtual ~IAudioPlayerService() = default;
};

#endif // IAUDIOPLAYERSERVICE_H
