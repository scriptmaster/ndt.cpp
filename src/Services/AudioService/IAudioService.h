#ifndef IAUDIOSERVICE_H
#define IAUDIOSERVICE_H

#include "../../App/DI/IService.h"

/**
 * IAudioService - Audio service interface
 * 
 * Manages audio generation, capture, and network subsystems
 */
class IAudioService : public IService {
public:
    virtual ~IAudioService() = default;
};

#endif // IAUDIOSERVICE_H
