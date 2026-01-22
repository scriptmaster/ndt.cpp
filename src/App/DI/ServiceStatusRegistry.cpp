#include "ServiceStatusRegistry.h"
#include <algorithm>
#include <chrono>
#include <mutex>

namespace {
std::mutex g_mutex;
std::vector<ServiceStatus> g_statuses;
bool g_timerStarted = false;
std::chrono::steady_clock::time_point g_startTime;
}

void ServiceStatusRegistry::Reset() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_statuses.clear();
    g_timerStarted = false;
}

void ServiceStatusRegistry::RegisterService(const std::string& name) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = std::find_if(g_statuses.begin(), g_statuses.end(),
                           [&](const ServiceStatus& status) { return status.name == name; });
    if (it == g_statuses.end()) {
        g_statuses.push_back({name, false});
    }
}

void ServiceStatusRegistry::MarkStarted(const std::string& name) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = std::find_if(g_statuses.begin(), g_statuses.end(),
                           [&](const ServiceStatus& status) { return status.name == name; });
    if (it != g_statuses.end()) {
        it->started = true;
    }
}

std::vector<ServiceStatus> ServiceStatusRegistry::GetStatuses() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_statuses;
}

bool ServiceStatusRegistry::AllStarted() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_statuses.empty()) {
        return false;
    }
    return std::all_of(g_statuses.begin(), g_statuses.end(),
                       [](const ServiceStatus& status) { return status.started; });
}

void ServiceStatusRegistry::StartTimer() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_timerStarted) {
        g_timerStarted = true;
        g_startTime = std::chrono::steady_clock::now();
    }
}

double ServiceStatusRegistry::ElapsedSeconds() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_timerStarted) {
        return 0.0;
    }
    auto elapsed = std::chrono::steady_clock::now() - g_startTime;
    return std::chrono::duration<double>(elapsed).count();
}
