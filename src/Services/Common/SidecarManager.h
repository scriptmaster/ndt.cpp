#pragma once

#include <string>

namespace SidecarManager {
    bool EnsureLlamaServerRunning();
    bool SendLlamaInference(const std::string& prompt, std::string& response);
}
