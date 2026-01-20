#ifndef HTTPSERVICE_H
#define HTTPSERVICE_H

#include "IHTTPService.h"

/**
 * HTTPService - HTTP client service implementation
 * 
 * Stub implementation for future HTTP/REST API functionality
 */
class HTTPService : public IHTTPService {
public:
    HTTPService();
    ~HTTPService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;
};

#endif // HTTPSERVICE_H
