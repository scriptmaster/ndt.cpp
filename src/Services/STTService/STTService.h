#ifndef STTSERVICE_H
#define STTSERVICE_H

#include "ISTTService.h"
#include <cstdint>
#include <string>
#include <vector>

/**
 * STTService - Speech-to-Text service implementation
 * 
 * Realtime Whisper STT
 */
struct whisper_context;

class STTService : public ISTTService {
public:
    STTService();
    ~STTService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;

    std::string Transcribe(const std::vector<float>& samples) override;
    std::string Transcribe(const float* samples, int count) override;
    std::string Transcribe(const int16_t* samples, int count) override;
    static STTService* GetInstance();

private:
    static STTService* instance_;
    struct whisper_context* ctx_;
    std::string modelPath_;
};

#endif // STTSERVICE_H
