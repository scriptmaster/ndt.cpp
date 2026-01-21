#ifndef WINDOWSERVICE_H
#define WINDOWSERVICE_H

#include "IWindowService.h"
#include <vector>

/**
 * WindowService - Window management service implementation
 * 
 * Manages GLFW windows, creation, configuration, and main loop
 */
class WindowService : public IWindowService {
public:
    WindowService();
    ~WindowService() override;

    void Configure() override;
    bool Start() override;
    void Stop() override;

    std::vector<WindowData>& GetWindows() override;
    const std::vector<WindowData>& GetWindows() const override;
    int RunLoop() override;
    void SetAdminStatus(bool isAdmin) override;
    void SetVSync(int interval) override;

private:
    std::vector<WindowData> windows_;
    bool isAdmin_;
    bool stopped_;
};

#endif // WINDOWSERVICE_H
