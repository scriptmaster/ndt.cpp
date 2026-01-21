#ifndef AUDIOGENERATION_H
#define AUDIOGENERATION_H

/**
 * AudioGeneration - Audio generation lifecycle
 */
void initAudioGeneration(int seed);
void cleanupAudio();
bool isAudioGenerationInitialized();

#endif // AUDIOGENERATION_H
