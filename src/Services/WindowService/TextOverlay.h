#pragma once

#include <string>
#include <vector>
#include "../../App/DI/ServiceStatusRegistry.h"

void renderBottomCenterTextOverlay(const std::string& text, int fbWidth, int fbHeight);
void renderServiceStatusOverlay(const std::vector<ServiceStatus>& statuses,
                                int fbWidth,
                                int fbHeight,
                                float startY,
                                double elapsedSeconds);