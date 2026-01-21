#include "AudioSeed.h"
#include "AudioGeneration.h"
#include <cstdio>

static int audioSeed = 12345;

int getAudioSeed() {
    return audioSeed;
}

void setAudioSeed(int seed) {
    audioSeed = seed;
    if (isAudioGenerationInitialized()) {
        initAudioGeneration(seed);
    }
}

bool saveAudioSeed(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "w");
    if (!file) {
        return false;
    }

    int result = fprintf(file, "%d\n", audioSeed);
    fclose(file);
    return (result > 0);
}

bool loadAudioSeed(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        return false;
    }

    int result = fscanf(file, "%d", &audioSeed);
    fclose(file);

    if (result != 1) {
        audioSeed = 12345;
        return false;
    }

    return true;
}
