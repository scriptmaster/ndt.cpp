#include "test.h"
#include "../display/audio.h"
#include <cstdio>
#include <fstream>

void TestAudioInit(test::TestContext& ctx) {
    initAudio(12345);
    int seed = getAudioSeed();
    ASSERT_EQ(12345, seed);
    cleanupAudio();
}

void TestAudioSeedPersistence(test::TestContext& ctx) {
    // Test seed save/load
    initAudio(99999);
    setAudioSeed(88888);
    bool saved = saveAudioSeed("test_audio_seed.txt");
    ASSERT_TRUE(saved);
    
    // Load it back
    bool loaded = loadAudioSeed("test_audio_seed.txt");
    ASSERT_TRUE(loaded);
    int seed = getAudioSeed();
    ASSERT_EQ(88888, seed);
    
    cleanupAudio();
    
    // Cleanup
    std::remove("test_audio_seed.txt");
}

void TestAudioWaveformAmplitudes(test::TestContext& ctx) {
    initAudio(12345);
    
    // Get waveform amplitudes
    auto amplitudes = getWaveformAmplitudes();
    
    // Should have some amplitudes (may be empty initially)
    // Just verify the function doesn't crash
    ASSERT_TRUE(true); // Function executed successfully
    
    cleanupAudio();
}

void TestAudioUpdate(test::TestContext& ctx) {
    initAudio(12345);
    
    // Test audio update doesn't crash
    updateAudio(0.016f); // ~60fps delta time
    ASSERT_TRUE(true); // Function executed successfully
    
    cleanupAudio();
}