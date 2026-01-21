#include "AudioGeneration.h"
#include <cstdlib>

static bool audioInitialized = false;

void initAudioGeneration(int seed) {
    audioInitialized = true;
    srand(seed);
}

void cleanupAudio() {
    audioInitialized = false;
}

bool isAudioGenerationInitialized() {
    return audioInitialized;
}
