#ifndef LLMSERVICE_H
#define LLMSERVICE_H

#include "ILLMService.h"

/**
 * LLMService - Large Language Model service implementation
 * 
 * Stub implementation for future LLM functionality
 */
class LLMService : public ILLMService {
public:
    LLMService();
    ~LLMService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;
};

#endif // LLMSERVICE_H
