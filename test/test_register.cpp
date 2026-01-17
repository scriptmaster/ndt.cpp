#include "test.h"

// Explicit test registration
// This avoids static initialization order issues

// Forward declarations
extern void TestSceneLoadCFileIO(test::TestContext& ctx);
extern void TestSceneLoadValidJSON(test::TestContext& ctx);
extern void TestSceneLoadInvalidFile(test::TestContext& ctx);
extern void TestSceneLoadWithHexColor(test::TestContext& ctx);
extern void TestSceneLoadWithWidgets(test::TestContext& ctx);
extern void TestSceneDefaultWaveform(test::TestContext& ctx);
extern void TestAudioInit(test::TestContext& ctx);
extern void TestAudioSeedPersistence(test::TestContext& ctx);
extern void TestAudioWaveformAmplitudes(test::TestContext& ctx);
extern void TestAudioUpdate(test::TestContext& ctx);
extern void TestColorHexParsing(test::TestContext& ctx);
extern void TestColorHexNoHash(test::TestContext& ctx);
extern void TestColorRGBParsing(test::TestContext& ctx);

void RegisterAllTests() {
    test::RegisterTest("SceneLoadCFileIO", TestSceneLoadCFileIO);
    test::RegisterTest("SceneLoadValidJSON", TestSceneLoadValidJSON);
    test::RegisterTest("SceneLoadInvalidFile", TestSceneLoadInvalidFile);
    test::RegisterTest("SceneLoadWithHexColor", TestSceneLoadWithHexColor);
    test::RegisterTest("SceneLoadWithWidgets", TestSceneLoadWithWidgets);
    test::RegisterTest("SceneDefaultWaveform", TestSceneDefaultWaveform);
    test::RegisterTest("AudioInit", TestAudioInit);
    test::RegisterTest("AudioSeedPersistence", TestAudioSeedPersistence);
    test::RegisterTest("AudioWaveformAmplitudes", TestAudioWaveformAmplitudes);
    test::RegisterTest("AudioUpdate", TestAudioUpdate);
    test::RegisterTest("ColorHexParsing", TestColorHexParsing);
    test::RegisterTest("ColorHexNoHash", TestColorHexNoHash);
    test::RegisterTest("ColorRGBParsing", TestColorRGBParsing);
}
