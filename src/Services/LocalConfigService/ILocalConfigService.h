#ifndef ILOCALCONFIGSERVICE_H
#define ILOCALCONFIGSERVICE_H

#include "../../App/DI/IService.h"
#include <string>

/**
 * ILocalConfigService - Local configuration service interface
 * 
 * Manages loading and saving configuration files
 * Provides access to application configuration values
 */
class ILocalConfigService : public IService {
public:
    virtual ~ILocalConfigService() = default;

    /**
     * Get audio seed value
     * @return Audio seed integer value
     */
    virtual int GetAudioSeed() const = 0;

    /**
     * Set audio seed value
     * @param seed Audio seed integer value
     */
    virtual void SetAudioSeed(int seed) = 0;

    /**
     * Load audio seed from config file
     * @param filename Config file path
     * @return true if loaded successfully, false otherwise
     */
    virtual bool LoadAudioSeed(const std::string& filename) = 0;

    /**
     * Save audio seed to config file
     * @param filename Config file path
     * @return true if saved successfully, false otherwise
     */
    virtual bool SaveAudioSeed(const std::string& filename) = 0;
};

#endif // ILOCALCONFIGSERVICE_H
