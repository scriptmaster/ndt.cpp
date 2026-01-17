#include "network.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wsock32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

static bool networkInitialized = false;

/**
 * Initialize network subsystem
 * On Windows, initializes WinSock2
 * On Unix, no special initialization needed
 */
bool initNetwork() {
    if (networkInitialized) {
        return true;
    }
    
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "[ERROR] Network: WSAStartup failed: " << result << std::endl;
        return false;
    }
    std::cout << "[DEBUG] Network: WinSock2 initialized" << std::endl;
#else
    // No special initialization needed on Unix
#endif
    
    networkInitialized = true;
    return true;
}

/**
 * Cleanup network subsystem
 * On Windows, cleans up WinSock2
 */
void cleanupNetwork() {
    if (!networkInitialized) {
        return;
    }
    
#ifdef _WIN32
    WSACleanup();
    std::cout << "[DEBUG] Network: WinSock2 cleaned up" << std::endl;
#else
    // No cleanup needed on Unix
#endif
    
    networkInitialized = false;
}

/**
 * Create WAV file header for audio data
 * WAV format: RIFF header + fmt chunk + data chunk
 */
static std::vector<char> createWAVHeader(int numSamples, int sampleRate, int channels = 1, int bitsPerSample = 16) {
    std::vector<char> header(44); // Standard WAV header size
    
    int dataSize = numSamples * channels * (bitsPerSample / 8);
    int fileSize = 36 + dataSize;
    
    // RIFF header
    memcpy(&header[0], "RIFF", 4);
    *((int*)&header[4]) = fileSize;
    memcpy(&header[8], "WAVE", 4);
    
    // fmt chunk
    memcpy(&header[12], "fmt ", 4);
    *((int*)&header[16]) = 16; // fmt chunk size
    *((short*)&header[20]) = 1; // PCM format
    *((short*)&header[22]) = channels;
    *((int*)&header[24]) = sampleRate;
    *((int*)&header[28]) = sampleRate * channels * (bitsPerSample / 8); // byte rate
    *((short*)&header[32]) = channels * (bitsPerSample / 8); // block align
    *((short*)&header[34]) = bitsPerSample;
    
    // data chunk
    memcpy(&header[36], "data", 4);
    *((int*)&header[40]) = dataSize;
    
    return header;
}

/**
 * Convert audio samples to WAV format
 * Creates complete WAV file in memory with header + audio data
 */
static std::vector<char> audioSamplesToWAV(const std::vector<short>& samples, int sampleRate) {
    // Create WAV header
    std::vector<char> wavData = createWAVHeader(samples.size(), sampleRate);
    
    // Append audio data
    size_t oldSize = wavData.size();
    wavData.resize(oldSize + samples.size() * sizeof(short));
    memcpy(&wavData[oldSize], samples.data(), samples.size() * sizeof(short));
    
    return wavData;
}

/**
 * Send HTTP POST request with multipart/form-data
 * Sends WAV file to Whisper STT server
 */
static bool sendHTTPPost(const std::vector<char>& body, const std::string& host, int port, const std::string& boundary) {
#ifdef _WIN32
    SOCKET sock = INVALID_SOCKET;
#else
    int sock = -1;
#endif
    
    // Create socket
#ifdef _WIN32
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[ERROR] Network: socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }
#else
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[ERROR] Network: socket creation failed" << std::endl;
        return false;
    }
#endif
    
    // Resolve hostname
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    // Try to parse as IP address first
#ifdef _WIN32
    // Windows: Use inet_addr for IP parsing
    unsigned long ipAddr = inet_addr(host.c_str());
    if (ipAddr != INADDR_NONE) {
        serverAddr.sin_addr.s_addr = ipAddr;
    } else {
        // Not an IP, try to resolve hostname
        struct hostent* hostEntry = gethostbyname(host.c_str());
        if (!hostEntry) {
            std::cerr << "[ERROR] Network: Failed to resolve hostname: " << host << std::endl;
            closesocket(sock);
            return false;
        }
        serverAddr.sin_addr = *((struct in_addr*)hostEntry->h_addr);
    }
#else
    // Unix: Use inet_pton for IP parsing
    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) != 1) {
        // Not an IP, try to resolve hostname
        struct hostent* hostEntry = gethostbyname(host.c_str());
        if (!hostEntry) {
            std::cerr << "[ERROR] Network: Failed to resolve hostname: " << host << std::endl;
            close(sock);
            return false;
        }
        serverAddr.sin_addr = *((struct in_addr*)hostEntry->h_addr);
    }
#endif
    
    // Connect to server
#ifdef _WIN32
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[ERROR] Network: connect failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        return false;
    }
#else
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "[ERROR] Network: connect failed" << std::endl;
        close(sock);
        return false;
    }
#endif
    
    // Build HTTP POST request
    std::ostringstream httpRequest;
    httpRequest << "POST /v1/audio/transcriptions HTTP/1.1\r\n";
    httpRequest << "Host: " << host << ":" << port << "\r\n";
    httpRequest << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";
    httpRequest << "Content-Length: " << body.size() << "\r\n";
    httpRequest << "Connection: close\r\n";
    httpRequest << "\r\n";
    
    std::string requestStr = httpRequest.str();
    
    // Send HTTP headers
#ifdef _WIN32
    int sent = send(sock, requestStr.c_str(), requestStr.length(), 0);
    if (sent == SOCKET_ERROR) {
        std::cerr << "[ERROR] Network: send headers failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        return false;
    }
#else
    ssize_t sent = send(sock, requestStr.c_str(), requestStr.length(), 0);
    if (sent < 0) {
        std::cerr << "[ERROR] Network: send headers failed" << std::endl;
        close(sock);
        return false;
    }
#endif
    
    // Send body
#ifdef _WIN32
    sent = send(sock, (const char*)body.data(), body.size(), 0);
    if (sent == SOCKET_ERROR) {
        std::cerr << "[ERROR] Network: send body failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        return false;
    }
#else
    sent = send(sock, (const char*)body.data(), body.size(), 0);
    if (sent < 0) {
        std::cerr << "[ERROR] Network: send body failed" << std::endl;
        close(sock);
        return false;
    }
#endif
    
    std::cout << "[DEBUG] Network: Sent " << sent << " bytes to Whisper STT" << std::endl;
    
    // Read response (simple - just read first 4KB)
    char buffer[4096];
#ifdef _WIN32
    int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        std::cout << "[DEBUG] Network: Whisper response: " << std::string(buffer, received).substr(0, 200) << std::endl;
    }
    closesocket(sock);
#else
    ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        std::cout << "[DEBUG] Network: Whisper response: " << std::string(buffer, received).substr(0, 200) << std::endl;
    }
    close(sock);
#endif
    
    return true;
}

/**
 * Send audio samples to Whisper STT server
 * Converts samples to WAV format and sends via HTTP POST
 */
bool sendAudioToWhisper(const std::vector<short>& audioSamples, int sampleRate, const std::string& serverHost, int serverPort) {
    if (!networkInitialized) {
        std::cerr << "[ERROR] Network: Not initialized" << std::endl;
        return false;
    }
    
    if (audioSamples.empty()) {
        std::cerr << "[WARNING] Network: No audio samples to send" << std::endl;
        return false;
    }
    
    // Convert audio samples to WAV format
    std::vector<char> wavData = audioSamplesToWAV(audioSamples, sampleRate);
    
    // Create multipart/form-data body for Whisper API
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::ostringstream multipartBody;
    
    // Form field: model
    multipartBody << "--" << boundary << "\r\n";
    multipartBody << "Content-Disposition: form-data; name=\"model\"\r\n";
    multipartBody << "\r\n";
    multipartBody << "whisper-1\r\n";
    
    // Form field: file
    multipartBody << "--" << boundary << "\r\n";
    multipartBody << "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n";
    multipartBody << "Content-Type: audio/wav\r\n";
    multipartBody << "\r\n";
    
    std::string multipartHeader = multipartBody.str();
    
    // Form field: response_format
    std::ostringstream multipartFooter;
    multipartFooter << "\r\n";
    multipartFooter << "--" << boundary << "\r\n";
    multipartFooter << "Content-Disposition: form-data; name=\"response_format\"\r\n";
    multipartFooter << "\r\n";
    multipartFooter << "json\r\n";
    multipartFooter << "--" << boundary << "--\r\n";
    
    std::string multipartTail = multipartFooter.str();
    
    // Combine: header + WAV data + footer
    std::vector<char> fullBody;
    fullBody.insert(fullBody.end(), multipartHeader.begin(), multipartHeader.end());
    fullBody.insert(fullBody.end(), wavData.begin(), wavData.end());
    fullBody.insert(fullBody.end(), multipartTail.begin(), multipartTail.end());
    
    // Send HTTP POST request
    return sendHTTPPost(fullBody, serverHost, serverPort, boundary);
}

/**
 * Send WAV file data to Whisper STT server
 * WAV data should include header + audio samples
 */
bool sendWAVToWhisper(const std::vector<char>& wavData, const std::string& serverHost, int serverPort) {
    if (!networkInitialized) {
        std::cerr << "[ERROR] Network: Not initialized" << std::endl;
        return false;
    }
    
    if (wavData.empty()) {
        std::cerr << "[WARNING] Network: No WAV data to send" << std::endl;
        return false;
    }
    
    // Create multipart/form-data body for Whisper API
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::ostringstream multipartBody;
    
    // Form field: model
    multipartBody << "--" << boundary << "\r\n";
    multipartBody << "Content-Disposition: form-data; name=\"model\"\r\n";
    multipartBody << "\r\n";
    multipartBody << "whisper-1\r\n";
    
    // Form field: file
    multipartBody << "--" << boundary << "\r\n";
    multipartBody << "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n";
    multipartBody << "Content-Type: audio/wav\r\n";
    multipartBody << "\r\n";
    
    std::string multipartHeader = multipartBody.str();
    
    // Form field: response_format
    std::ostringstream multipartFooter;
    multipartFooter << "\r\n";
    multipartFooter << "--" << boundary << "\r\n";
    multipartFooter << "Content-Disposition: form-data; name=\"response_format\"\r\n";
    multipartFooter << "\r\n";
    multipartFooter << "json\r\n";
    multipartFooter << "--" << boundary << "--\r\n";
    
    std::string multipartTail = multipartFooter.str();
    
    // Combine: header + WAV data + footer
    std::vector<char> fullBody;
    fullBody.insert(fullBody.end(), multipartHeader.begin(), multipartHeader.end());
    fullBody.insert(fullBody.end(), wavData.begin(), wavData.end());
    fullBody.insert(fullBody.end(), multipartTail.begin(), multipartTail.end());
    
    // Send HTTP POST request
    return sendHTTPPost(fullBody, serverHost, serverPort, boundary);
}
