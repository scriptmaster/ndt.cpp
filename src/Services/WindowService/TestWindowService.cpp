#include "TestWindowService.h"
#include <chrono>
#include <iostream>
#include <thread>

TestWindowService::TestWindowService() {
}

TestWindowService::~TestWindowService() {
}

void TestWindowService::Configure() {
}

bool TestWindowService::Start() {
    std::cout << "[DEBUG] TestWindowService started (headless)" << std::endl;
    return true;
}

void TestWindowService::Stop() {
    std::cout << "[DEBUG] TestWindowService stopped" << std::endl;
}

std::vector<WindowData>& TestWindowService::GetWindows() {
    return windows_;
}

const std::vector<WindowData>& TestWindowService::GetWindows() const {
    return windows_;
}

int TestWindowService::RunLoop() {
    using namespace std::chrono_literals;
    std::cout << "[DEBUG] TestWindowService RunLoop start (10s)" << std::endl;
    for (int i = 0; i < 100; i++) {
        std::this_thread::sleep_for(100ms);
    }
    std::cout << "[DEBUG] TestWindowService RunLoop end" << std::endl;
    return 0;
}

void TestWindowService::SetAdminStatus(bool) {
}

void TestWindowService::SetVSync(int) {
}
