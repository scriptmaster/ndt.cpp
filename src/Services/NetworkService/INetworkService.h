#ifndef INETWORKSERVICE_H
#define INETWORKSERVICE_H

#include "../../App/DI/IService.h"

/**
 * INetworkService - Network initialization service interface
 */
class INetworkService : public IService {
public:
    virtual ~INetworkService() = default;
};

#endif // INETWORKSERVICE_H
