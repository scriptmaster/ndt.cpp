#ifndef NETWORK_SYSTEM_H
#define NETWORK_SYSTEM_H

#include <string>
#include <vector>

/**
 * NetworkSystem - Network subsystem functions
 * 
 * Single responsibility: Network initialization and Whisper STT client
 */

// Network lifecycle
bool initNetwork();
void cleanupNetwork();

// Send audio to Whisper STT
bool sendAudioToWhisper(const std::vector<short>& audioSamples, int sampleRate, 
                         const std::string& serverHost = "localhost", int serverPort = 8070);
bool sendWAVToWhisper(const std::vector<char>& wavData, const std::string& serverHost = "localhost", int serverPort = 8070);

#endif // NETWORK_SYSTEM_H
