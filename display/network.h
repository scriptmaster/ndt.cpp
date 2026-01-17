#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>

// Network client for Whisper STT
// Sends audio data to Whisper STT server on port 8070
bool initNetwork();
void cleanupNetwork();
bool sendAudioToWhisper(const std::vector<short>& audioSamples, int sampleRate, const std::string& serverHost = "localhost", int serverPort = 8070);
bool sendWAVToWhisper(const std::vector<char>& wavData, const std::string& serverHost = "localhost", int serverPort = 8070);

#endif // NETWORK_H
