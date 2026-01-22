#ifndef SERVICESTATUSREGISTRY_H
#define SERVICESTATUSREGISTRY_H

#include <string>
#include <vector>

struct ServiceStatus {
    std::string name;
    bool started;
};

class ServiceStatusRegistry {
public:
    static void Reset();
    static void RegisterService(const std::string& name);
    static void MarkStarted(const std::string& name);
    static std::vector<ServiceStatus> GetStatuses();
    static bool AllStarted();
    static void StartTimer();
    static double ElapsedSeconds();

private:
    ServiceStatusRegistry() = default;
};

#endif // SERVICESTATUSREGISTRY_H
