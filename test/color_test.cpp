#include "test.h"
#include "../src/Services/WindowService/Scene.h"
#include <cstdio>
#include <fstream>

// Helper to test color parsing (we need to access parseColor)
// Since parseColor is static in scene.cpp, we test it indirectly through loadScene

void TestColorHexParsing(test::TestContext& ctx) {
    // Create test file with hex color
    const char* test_file = "test_color_hex.json";
    {
        std::ofstream file(test_file);
        file << "{\n";
        file << "  \"id\": \"test\",\n";
        file << "  \"layout\": \"grid\",\n";
        file << "  \"cols\": 8,\n";
        file << "  \"rows\": 12,\n";
        file << "  \"bg\": {\n";
        file << "    \"color\": \"#ABCDEF\"\n";
        file << "  }\n";
        file << "}\n";
    }
    
    Scene scene;
    bool result = loadScene(test_file, scene);
    ASSERT_TRUE(result);
    ASSERT_STR_EQ("#ABCDEF", scene.bg.color);
    
    // Cleanup
    std::remove(test_file);
}

void TestColorHexNoHash(test::TestContext& ctx) {
    // Test hex color without hash
    const char* test_file = "test_color_hex2.json";
    {
        std::ofstream file(test_file);
        file << "{\n";
        file << "  \"id\": \"test\",\n";
        file << "  \"layout\": \"grid\",\n";
        file << "  \"cols\": 8,\n";
        file << "  \"rows\": 12,\n";
        file << "  \"bg\": {\n";
        file << "    \"color\": \"ABCDEF\"\n";
        file << "  }\n";
        file << "}\n";
    }
    
    Scene scene;
    bool result = loadScene(test_file, scene);
    ASSERT_TRUE(result);
    // Should handle hex without hash (or preserve as-is)
    ASSERT_TRUE(!scene.bg.color.empty());
    
    // Cleanup
    std::remove(test_file);
}

void TestColorRGBParsing(test::TestContext& ctx) {
    // Test RGB comma-separated format
    const char* test_file = "test_color_rgb.json";
    {
        std::ofstream file(test_file);
        file << "{\n";
        file << "  \"id\": \"test\",\n";
        file << "  \"layout\": \"grid\",\n";
        file << "  \"cols\": 8,\n";
        file << "  \"rows\": 12,\n";
        file << "  \"bg\": {\n";
        file << "    \"color\": \"0.5,0.6,0.7\"\n";
        file << "  }\n";
        file << "}\n";
    }
    
    Scene scene;
    bool result = loadScene(test_file, scene);
    ASSERT_TRUE(result);
    ASSERT_STR_EQ("0.5,0.6,0.7", scene.bg.color);
    
    // Cleanup
    std::remove(test_file);
}