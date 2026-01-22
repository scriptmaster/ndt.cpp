#include "SidecarManager.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {
constexpr int kLlamaPort = 8070;
constexpr const char* kLlamaHost = "127.0.0.1";
constexpr const char* kLlamaHealthPath = "/health";
constexpr const char* kLlamaModelsPath = "/v1/models";

#ifdef _WIN32
bool ensureWinsock() {
    static bool initialized = false;
    if (initialized) {
        return true;
    }
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
    initialized = true;
    return true;
}
#endif

int httpGetStatus(const std::string& host, int port, const std::string& path) {
#ifdef _WIN32
    if (!ensureWinsock()) {
        return 0;
    }
#endif
    int sockfd = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
    if (sockfd < 0) {
        return 0;
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return 0;
    }

    const int connectResult = connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (connectResult < 0) {
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return 0;
    }

    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n"
            << "Host: " << host << "\r\n"
            << "Connection: close\r\n\r\n";
    const std::string req = request.str();
    send(sockfd, req.c_str(), static_cast<int>(req.size()), 0);

    char buffer[512];
    int received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return 0;
    }
    buffer[received] = '\0';

    int status = 0;
    std::string firstLine(buffer);
    const size_t lineEnd = firstLine.find("\r\n");
    if (lineEnd != std::string::npos) {
        firstLine = firstLine.substr(0, lineEnd);
    }
    std::istringstream lineStream(firstLine);
    std::string httpVersion;
    lineStream >> httpVersion >> status;

#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
    return status;
}

bool isLlamaReady() {
    int status = httpGetStatus(kLlamaHost, kLlamaPort, kLlamaHealthPath);
    if (status == 200) {
        return true;
    }
    status = httpGetStatus(kLlamaHost, kLlamaPort, kLlamaModelsPath);
    return status == 200;
}

void killProcessOnPort(int port) {
#ifdef _WIN32
    std::ostringstream cmd;
    cmd << "for /f \"tokens=5\" %a in ('netstat -ano ^| findstr :"
        << port
        << " ^| findstr LISTENING') do taskkill /PID %a /F >nul 2>&1";
    system(cmd.str().c_str());
#else
    std::ostringstream cmd;
    cmd << "lsof -ti :" << port << " | xargs -r kill -9";
    system(cmd.str().c_str());
#endif
}

std::string findLlamaModel() {
    const char* envModel = std::getenv("LLAMA_MODEL_PATH");
    if (envModel && std::filesystem::exists(envModel)) {
        return std::string(envModel);
    }

    const std::vector<std::string> dirs = {
        "models/llama",
        "models",
        "../models/llama",
        "../models",
        "../../models/llama",
        "../../models",
    };

    for (const auto& dir : dirs) {
        if (!std::filesystem::exists(dir)) {
            continue;
        }
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const auto path = entry.path();
            if (path.extension() == ".gguf") {
                return path.string();
            }
        }
    }

    return "";
}

bool launchLlamaServer() {
    const std::string modelPath = findLlamaModel();
    if (modelPath.empty()) {
        std::cerr << "[ERROR] LLM: No GGUF model found for llama-server. "
                  << "Set LLAMA_MODEL_PATH or add a model under models/llama." << std::endl;
        return false;
    }

    std::ostringstream cmd;
#ifdef _WIN32
    cmd << "cmd.exe /c start \"llama-server\" /B llama-server"
        << " --port " << kLlamaPort
        << " --host " << kLlamaHost
        << " -m \"" << modelPath << "\""
        << " > logs/llama-server.log 2>&1";
#else
    cmd << "llama-server --port " << kLlamaPort
        << " --host " << kLlamaHost
        << " -m \"" << modelPath << "\""
        << " > logs/llama-server.log 2>&1 &";
#endif
    return system(cmd.str().c_str()) == 0;
}

} // namespace

namespace SidecarManager {

bool EnsureLlamaServerRunning() {
    if (isLlamaReady()) {
        std::cout << "[DEBUG] LLM: llama-server ready on port " << kLlamaPort << std::endl;
        return true;
    }

    killProcessOnPort(kLlamaPort);

    if (!launchLlamaServer()) {
        return false;
    }

    const int maxAttempts = 20;
    for (int i = 0; i < maxAttempts; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        if (isLlamaReady()) {
            std::cout << "[DEBUG] LLM: llama-server started on port " << kLlamaPort << std::endl;
            return true;
        }
    }

    std::cerr << "[ERROR] LLM: llama-server not ready after timeout" << std::endl;
    return false;
}

bool SendLlamaInference(const std::string& prompt, std::string& response) {
    (void)prompt;
    response.clear();
    return false;
}

} // namespace SidecarManager
