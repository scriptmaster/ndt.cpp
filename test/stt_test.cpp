#include "test.h"
#include "../src/Services/STTService/STTService.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {
bool loadWavPcm16(const std::string& path, std::vector<int16_t>& outPcm, int& outSampleRate) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    auto readU32 = [&](uint32_t& out) { file.read(reinterpret_cast<char*>(&out), sizeof(out)); };
    auto readU16 = [&](uint16_t& out) { file.read(reinterpret_cast<char*>(&out), sizeof(out)); };

    char riff[4] = {};
    file.read(riff, 4);
    uint32_t riffSize = 0;
    readU32(riffSize);
    char wave[4] = {};
    file.read(wave, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0 || std::strncmp(wave, "WAVE", 4) != 0) {
        return false;
    }

    uint16_t audioFormat = 0;
    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataSize = 0;
    std::streampos dataPos = 0;

    while (file && !dataPos) {
        char chunkId[4] = {};
        uint32_t chunkSize = 0;
        file.read(chunkId, 4);
        readU32(chunkSize);
        if (std::strncmp(chunkId, "fmt ", 4) == 0) {
            readU16(audioFormat);
            readU16(numChannels);
            readU32(sampleRate);
            uint32_t byteRate = 0;
            uint16_t blockAlign = 0;
            readU32(byteRate);
            readU16(blockAlign);
            readU16(bitsPerSample);
            if (chunkSize > 16) {
                file.seekg(chunkSize - 16, std::ios::cur);
            }
        } else if (std::strncmp(chunkId, "data", 4) == 0) {
            dataSize = chunkSize;
            dataPos = file.tellg();
            file.seekg(chunkSize, std::ios::cur);
        } else {
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    if (!dataPos || audioFormat != 1 || bitsPerSample != 16 || sampleRate == 0 || numChannels == 0) {
        return false;
    }

    file.clear();
    file.seekg(dataPos);
    std::vector<int16_t> pcm(dataSize / sizeof(int16_t));
    file.read(reinterpret_cast<char*>(pcm.data()), dataSize);

    outPcm.clear();
    outPcm.reserve(pcm.size() / numChannels);
    for (size_t i = 0; i + (numChannels - 1) < pcm.size(); i += numChannels) {
        int32_t sum = 0;
        for (uint16_t ch = 0; ch < numChannels; ch++) {
            sum += pcm[i + ch];
        }
        outPcm.push_back(static_cast<int16_t>(sum / static_cast<int32_t>(numChannels)));
    }

    outSampleRate = static_cast<int>(sampleRate);
    return true;
}

bool hasMeaningfulText(const std::string& text) {
    for (char c : text) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            return true;
        }
    }
    return false;
}
} // namespace

void TestSTTServiceTranscribe(test::TestContext& ctx) {
    std::vector<int16_t> pcm;
    int sampleRate = 0;
    ASSERT_TRUE(loadWavPcm16("test.wav", pcm, sampleRate));
    ASSERT_TRUE(!pcm.empty());
    ASSERT_EQ(16000, sampleRate);

    STTService stt;
    stt.Configure();
    ASSERT_TRUE(stt.Start());

    std::string result = stt.Transcribe(pcm.data(), static_cast<int>(pcm.size()));
    std::cout << "[TEST] STT result: " << result << std::endl;
    ASSERT_TRUE(hasMeaningfulText(result));
}
