#ifndef STTSERVICE_H
#define STTSERVICE_H

#include "ISTTService.h"

/**
 * STTService - Speech-to-Text service implementation
 * 
 * Stub implementation for future Whisper STT functionality
 */
class STTService : public ISTTService {
public:
    STTService();
    ~STTService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;
};

#endif // STTSERVICE_H
