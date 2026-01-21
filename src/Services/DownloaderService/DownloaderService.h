#ifndef DOWNLOADERSERVICE_H
#define DOWNLOADERSERVICE_H

#include <string>

/**
 * DownloaderService - Windows-only file downloader (WinINet)
 *
 * Provides minimal download support for model files.
 */
class DownloaderService {
public:
    static std::string downloadHFModel(const std::string& url, const std::string& localPath);
};

#endif // DOWNLOADERSERVICE_H
