#ifndef ISTTSERVICE_H
#define ISTTSERVICE_H

#include "../../App/DI/IService.h"
#include <string>
#include <vector>

/**
 * ISTTService - Speech-to-Text service interface
 * 
 * Provides speech-to-text functionality (e.g., Whisper STT)
 * Stub implementation for future use
 */
class ISTTService : public IService {
public:
    virtual ~ISTTService() = default;
};

#endif // ISTTSERVICE_H
