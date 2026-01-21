#ifndef AUDIOSEED_H
#define AUDIOSEED_H

#include <string>

/**
 * AudioSeed - Seed persistence and access
 */
int getAudioSeed();
void setAudioSeed(int seed);
bool loadAudioSeed(const std::string& filename);
bool saveAudioSeed(const std::string& filename);

#endif // AUDIOSEED_H
