#ifndef ISTTSERVICE_H
#define ISTTSERVICE_H

#include "../../App/DI/IService.h"
#include <cstdint>
#include <string>
#include <vector>

/**
 * ISTTService - Speech-to-Text service interface
 * 
 * Provides speech-to-text functionality (Whisper STT)
 */
class ISTTService : public IService {
public:
    virtual ~ISTTService() = default;
    virtual std::string Transcribe(const std::vector<float>& samples) = 0;
    virtual std::string Transcribe(const float* samples, int count) = 0;
    virtual std::string Transcribe(const int16_t* samples, int count) = 0;
};

#endif // ISTTSERVICE_H
