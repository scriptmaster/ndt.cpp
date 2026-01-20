#ifndef TTSSERVICE_H
#define TTSSERVICE_H

#include "ITTSService.h"

/**
 * TTSService - Text-to-Speech service implementation
 * 
 * Stub implementation for future TTS functionality
 */
class TTSService : public ITTSService {
public:
    TTSService();
    ~TTSService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;
};

#endif // TTSSERVICE_H
