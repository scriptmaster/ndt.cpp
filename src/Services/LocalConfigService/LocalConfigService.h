#ifndef LOCALCONFIGSERVICE_H
#define LOCALCONFIGSERVICE_H

#include "ILocalConfigService.h"
#include <string>

/**
 * LocalConfigService - Local configuration service implementation
 * 
 * Manages configuration files (e.g., audio_seed.txt)
 * Provides default values when config files are missing
 */
class LocalConfigService : public ILocalConfigService {
public:
    LocalConfigService();
    ~LocalConfigService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;

    int GetAudioSeed() const override;
    void SetAudioSeed(int seed) override;
    bool LoadAudioSeed(const std::string& filename) override;
    bool SaveAudioSeed(const std::string& filename) override;

private:
    int audioSeed_;
    static const int DEFAULT_AUDIO_SEED = 12345;
};

#endif // LOCALCONFIGSERVICE_H
