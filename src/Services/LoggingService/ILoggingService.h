#ifndef ILOGGINGSERVICE_H
#define ILOGGINGSERVICE_H

#include "../../App/DI/IService.h"

/**
 * ILoggingService - Logging service interface
 * 
 * Provides logging capabilities to the application
 * Special service: initializes logging system in constructor
 */
class ILoggingService : public IService {
public:
    virtual ~ILoggingService() = default;
};

#endif // ILOGGINGSERVICE_H
