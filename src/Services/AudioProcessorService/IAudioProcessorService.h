#ifndef IAUDIOPROCESSORSERVICE_H
#define IAUDIOPROCESSORSERVICE_H

#include "../../App/DI/IService.h"
#include <vector>

/**
 * IAudioProcessorService - Speech chunking service interface
 */
class IAudioProcessorService : public IService {
public:
    virtual ~IAudioProcessorService() = default;

    virtual bool PopSpeechChunk(std::vector<float>& outSamples) = 0;
};

#endif // IAUDIOPROCESSORSERVICE_H
