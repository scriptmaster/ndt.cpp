#include "NetworkService.h"
#include "NetworkSystem.h"
#include <iostream>

NetworkService::NetworkService() {
}

NetworkService::~NetworkService() {
}

void NetworkService::Configure() {
    // No configuration needed
}

bool NetworkService::Start() {
    std::cout << "[DEBUG] Initializing network..." << std::endl;
    try {
        if (!initNetwork()) {
            std::cerr << "[WARNING] Network initialization failed - STT will not work" << std::endl;
        } else {
            std::cout << "[DEBUG] Network initialized - SUCCESS" << std::endl;
        }
        return true;
    } catch (...) {
        std::cerr << "[ERROR] Network initialization failed with exception" << std::endl;
        return true;
    }
}

void NetworkService::Stop() {
    std::cout << "[DEBUG] Stopping network..." << std::endl;
    cleanupNetwork();
    std::cout << "[DEBUG] Network stopped - SUCCESS" << std::endl;
}
