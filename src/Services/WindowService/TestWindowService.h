#ifndef TESTWINDOWSERVICE_H
#define TESTWINDOWSERVICE_H

#include "IWindowService.h"

/**
 * TestWindowService - Headless window service for test mode
 * Runs a short loop without initializing GLFW.
 */
class TestWindowService : public IWindowService {
public:
    TestWindowService();
    ~TestWindowService() override;

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
};

#endif // TESTWINDOWSERVICE_H
