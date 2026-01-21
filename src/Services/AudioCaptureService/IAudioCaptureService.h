#ifndef IAUDIOCAPTURESERVICE_H
#define IAUDIOCAPTURESERVICE_H

#include "../../App/DI/IService.h"

/**
 * IAudioCaptureService - Audio capture service interface
 */
class IAudioCaptureService : public IService {
public:
    virtual ~IAudioCaptureService() = default;

    virtual bool DequeueAudio(float* out, int maxSamples) = 0;
};

#endif // IAUDIOCAPTURESERVICE_H
