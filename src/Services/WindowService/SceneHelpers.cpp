#include "SceneHelpers.h"
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdio>

/**
 * SceneHelpers implementation
 * JSON parsing helpers and color parsing
 */

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::string extractStringValue(const std::string& line) {
    size_t colon = line.find(':');
    if (colon == std::string::npos) return "";
    size_t startQuote = line.find('"', colon);
    if (startQuote == std::string::npos) return "";
    size_t start = startQuote + 1;
    if (start >= line.length()) return "";
    size_t end = line.find('"', start);
    if (end == std::string::npos) return "";
    if (end <= start) return "";
    return line.substr(start, end - start);
}

int extractIntValue(const std::string& line) {
    size_t colon = line.find(':');
    if (colon == std::string::npos) return 0;
    std::string value = trim(line.substr(colon + 1));
    value.erase(std::remove(value.begin(), value.end(), ','), value.end());
    if (value.empty()) return 0;
    try {
        return std::stoi(value);
    } catch (...) {
        return 0;
    }
}

float extractFloatValue(const std::string& line) {
    size_t colon = line.find(':');
    if (colon == std::string::npos) return 0.0f;
    std::string value = trim(line.substr(colon + 1));
    value.erase(std::remove(value.begin(), value.end(), ','), value.end());
    if (value.empty()) return 0.0f;
    try {
        return std::stof(value);
    } catch (...) {
        return 0.0f;
    }
}

void parseColor(const std::string& colorStr, float& r, float& g, float& b) {
    r = g = b = 0.1f;
    
    if (colorStr.empty()) return;
    
    // Check if hex format (#ffffff or ffffff)
    std::string hexStr = colorStr;
    if (hexStr[0] == '#') {
        hexStr = hexStr.substr(1);
    }
    
    // Try to parse as hex (6 hex digits)
    if (hexStr.length() == 6) {
        bool isHex = true;
        for (char c : hexStr) {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                isHex = false;
                break;
            }
        }
        
        if (isHex) {
            unsigned int hexValue = std::stoul(hexStr, nullptr, 16);
            r = ((hexValue >> 16) & 0xFF) / 255.0f;
            g = ((hexValue >> 8) & 0xFF) / 255.0f;
            b = (hexValue & 0xFF) / 255.0f;
            return;
        }
    }
    
    // Fallback: try comma-separated format "r,g,b"
    size_t comma1 = colorStr.find(',');
    size_t comma2 = colorStr.find(',', comma1 + 1);
    if (comma1 != std::string::npos && comma2 != std::string::npos) {
        try {
            r = std::stof(colorStr.substr(0, comma1));
            g = std::stof(colorStr.substr(comma1 + 1, comma2 - comma1 - 1));
            b = std::stof(colorStr.substr(comma2 + 1));
            // Normalize if values are > 1.0 (assumed 0-255 range)
            if (r > 1.0f || g > 1.0f || b > 1.0f) {
                r /= 255.0f;
                g /= 255.0f;
                b /= 255.0f;
            }
        } catch (...) {
            r = g = b = 0.1f;
        }
    }
}

std::string resolveScenePath(const std::string& preferredPath, const std::string& legacyPath) {
    FILE* file = fopen(preferredPath.c_str(), "r");
    if (file) {
        fclose(file);
        return preferredPath;
    }

    file = fopen(legacyPath.c_str(), "r");
    if (file) {
        fclose(file);
        return legacyPath;
    }

    return preferredPath;
}
