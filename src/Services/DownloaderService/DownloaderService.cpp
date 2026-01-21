#include "DownloaderService.h"

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#endif

#include <filesystem>
#include <fstream>
#include <iostream>

std::string DownloaderService::downloadHFModel(const std::string& url, const std::string& localPath) {
    namespace fs = std::filesystem;

    if (localPath.empty()) {
        return "";
    }

    std::error_code ec;
    if (fs::exists(localPath, ec)) {
        return localPath;
    }

    fs::path path(localPath);
    fs::path parent = path.parent_path();
    if (!parent.empty()) {
        fs::create_directories(parent, ec);
    }

#ifndef _WIN32
    std::cerr << "[ERROR] DownloaderService: WinINet not available on this platform" << std::endl;
    return "";
#else
    HINTERNET hInternet = InternetOpenA("ndt.cpp", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        std::cerr << "[ERROR] DownloaderService: InternetOpen failed" << std::endl;
        return "";
    }

    HINTERNET hUrl = InternetOpenUrlA(
        hInternet,
        url.c_str(),
        NULL,
        0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE,
        0);
    if (!hUrl) {
        std::cerr << "[ERROR] DownloaderService: InternetOpenUrl failed" << std::endl;
        InternetCloseHandle(hInternet);
        return "";
    }

    std::ofstream out(localPath, std::ios::binary);
    if (!out) {
        std::cerr << "[ERROR] DownloaderService: Failed to create " << localPath << std::endl;
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return "";
    }

    char buffer[8192];
    DWORD bytesRead = 0;
    bool ok = true;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        out.write(buffer, static_cast<std::streamsize>(bytesRead));
        if (!out.good()) {
            ok = false;
            break;
        }
    }

    out.close();
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    if (!ok || !fs::exists(localPath, ec)) {
        std::cerr << "[ERROR] DownloaderService: Download failed for " << localPath << std::endl;
        fs::remove(localPath, ec);
        return "";
    }

    return localPath;
#endif
}
