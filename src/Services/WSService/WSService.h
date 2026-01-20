#ifndef WSSERVICE_H
#define WSSERVICE_H

#include "IWSService.h"

/**
 * WSService - WebSocket service implementation
 * 
 * Stub implementation for future WebSocket functionality
 */
class WSService : public IWSService {
public:
    WSService();
    ~WSService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;
};

#endif // WSSERVICE_H
