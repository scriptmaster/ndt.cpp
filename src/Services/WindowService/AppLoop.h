#ifndef APPLOOP_H
#define APPLOOP_H

#include "WindowData.h"
#include <vector>

/**
 * AppLoop - Main application loop and input handling
 * 
 * Single responsibility: Run main application loop, process input, maintain window visibility
 * Ported from display/app.cpp
 */

/**
 * Check if user requested application shutdown
 * @param windows Vector of all windows to check
 * @return true if any window should close
 */
bool shouldShutdownApplication(const std::vector<WindowData>& windows);

/**
 * Process all user input for a single frame
 * Checks for keyboard shortcuts like Alt+F4 across all windows
 * @param windows Vector of all windows to check for input
 */
void processUserInput(const std::vector<WindowData>& windows);

/**
 * Update window visibility and focus state
 * Ensures windows remain visible if they get minimized or hidden
 * @param windows Vector of all windows to maintain visibility for
 */
void maintainWindowVisibility(std::vector<WindowData>& windows);

/**
 * Run the main application loop
 * Continues until shutdown is requested or all windows are closed
 * @param windows Vector of all windows to render
 */
void runMainLoop(std::vector<WindowData>& windows);

#endif // APPLOOP_H
