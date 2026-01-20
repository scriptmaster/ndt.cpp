#ifndef IWINDOWSERVICE_H
#define IWINDOWSERVICE_H

#include "../../App/DI/IService.h"
#include "WindowData.h"
#include <vector>

/**
 * IWindowService - Window management service interface
 * 
 * Manages GLFW windows and window lifecycle
 */
class IWindowService : public IService {
public:
    virtual ~IWindowService() = default;

    /**
     * Get all application windows
     * @return Vector of window data structures
     */
    virtual std::vector<WindowData>& GetWindows() = 0;
    virtual const std::vector<WindowData>& GetWindows() const = 0;

    /**
     * Run the main application loop
     * This owns the main rendering loop
     * @return Exit code (0 for success, non-zero for errors)
     */
    virtual int RunLoop() = 0;

    /**
     * Set admin status for all windows
     * @param isAdmin True if running as admin
     */
    virtual void SetAdminStatus(bool isAdmin) = 0;

    /**
     * Configure VSync
     * @param interval VSync interval (1 = enabled, 0 = disabled)
     */
    virtual void SetVSync(int interval) = 0;
};

#endif // IWINDOWSERVICE_H
