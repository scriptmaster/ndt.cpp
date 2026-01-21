#ifndef NETWORKSERVICE_H
#define NETWORKSERVICE_H

#include "INetworkService.h"

/**
 * NetworkService - Network subsystem service
 */
class NetworkService : public INetworkService {
public:
    NetworkService();
    ~NetworkService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;
};

#endif // NETWORKSERVICE_H
