#include "Scene.h"
#include "SceneHelpers.h"
#include "BackgroundGraphics.h"
#include "../AudioCaptureService/AudioWaveform.h"
#include "../AudioCaptureService/AudioCapture.h"
#include <iostream>
#include <cmath>
#include <vector>

#ifdef _WIN32
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

// Forward declarations for dependencies to be ported
extern void logSceneRender(int frameCount, int fbWidth, int fbHeight, int state, 
                           float deltaTime, const std::string& bgGraphic, int widgetCount);

/**
 * SceneRenderer - Scene rendering functions
 * 
 * Single responsibility: Render scenes, widgets, and waveform
 */

// Render a single language card widget
static void renderLanguageCard(const Widget& widget, float x, float y, float w, float h) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.2f, 0.25f, 0.3f, 0.8f);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();
    
    glColor4f(0.4f, 0.5f, 0.6f, 0.9f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();
    
    std::string lang = widget.properties.count("language") ? widget.properties.at("language") : "";
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glPointSize(10.0f);
    glBegin(GL_POINTS);
        glVertex2f(x + w * 0.5f, y + h * 0.5f);
    glEnd();
    glDisable(GL_BLEND);
}

void renderScene(const Scene& scene, int windowWidth, int windowHeight, float deltaTime, int frameCount) {
    try {
        if (scene.cols <= 0 || scene.rows <= 0 || windowWidth <= 0 || windowHeight <= 0) {
            std::cerr << "[ERROR] renderScene: Invalid dimensions" << std::endl;
            return;
        }
        
        float cellWidth = (float)windowWidth / scene.cols;
        float cellHeight = (float)windowHeight / scene.rows;
    
        glViewport(0, 0, windowWidth, windowHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, windowWidth, 0.0, windowHeight, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        float bgR = 1.0f, bgG = 1.0f, bgB = 1.0f;
        if (!scene.bg.color.empty()) {
            parseColor(scene.bg.color, bgR, bgG, bgB);
        }
        glClearColor(bgR, bgG, bgB, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        initBackgroundGraphics(windowWidth, windowHeight);
        
        if (scene.bg.graphic == "triangles") {
            renderTriangles(windowWidth, windowHeight, deltaTime);
        } else if (scene.bg.graphic == "dots_lines") {
            renderDotsWithLines(windowWidth, windowHeight, deltaTime);
        } else if (scene.bg.graphic == "blurred_orbs") {
            renderBlurredOrbs(windowWidth, windowHeight, deltaTime);
        }
        
        logSceneRender(frameCount, windowWidth, windowHeight, 3, deltaTime, scene.bg.graphic, scene.widgets.size());
        
        for (const auto& widget : scene.widgets) {
            if (widget.type == "language_card") {
                float x = widget.col * cellWidth;
                float y = (scene.rows - widget.row - widget.height) * cellHeight;
                float w = widget.width * cellWidth;
                float h = widget.height * cellHeight;
                
                float marginX = w * widget.margin;
                float marginY = h * widget.margin;
                x += marginX;
                y += marginY;
                w -= marginX * 2;
                h -= marginY * 2;
                
                renderLanguageCard(widget, x, y, w, h);
            }
        }
        
        try {
            renderWaveformWidget(windowWidth, windowHeight);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception rendering waveform: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] Unknown exception rendering waveform" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in renderScene: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception in renderScene" << std::endl;
    }
}

void renderWaveformWidget(int windowWidth, int windowHeight) {
    std::vector<float> barHeights = getWaveformAmplitudes();
    if (barHeights.empty()) return;
    
    float widgetHeight = windowHeight * 0.12f;
    float widgetBottom = 0.0f;
    float barSpacing = 0.001f * windowWidth;
    float barWidth = 3.0f;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.2f, 0.8f, 1.0f, 0.8f);
    float x = windowWidth;
    
    for (size_t i = 0; i < barHeights.size(); i++) {
        float barHeight = barHeights[i] * widgetHeight;
        float barX = x - barWidth - barSpacing;
        float barY = widgetBottom + (widgetHeight - barHeight) * 0.5f;
        
        if (barX >= 0 && barHeight > 0.1f) {
            glBegin(GL_QUADS);
                glVertex2f(barX, barY);
                glVertex2f(barX + barWidth, barY);
                glVertex2f(barX + barWidth, barY + barHeight);
                glVertex2f(barX, barY + barHeight);
            glEnd();
        }
        
        x = barX;
        if (x < 0) break;
    }
    
    glDisable(GL_BLEND);
    renderDeviceNameLabel(windowWidth, windowHeight);
}

void renderDeviceNameLabel(int windowWidth, int windowHeight) {
    (void)windowHeight;
    std::string deviceName = getAudioDeviceName();
    if (deviceName.empty()) return;
    
    float labelWidth = deviceName.length() * 8.0f;
    float labelHeight = 20.0f;
    float margin = 10.0f;
    float labelX = windowWidth - labelWidth - margin;
    float labelY = margin;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
        glVertex2f(labelX - 5.0f, labelY - 2.0f);
        glVertex2f(windowWidth - 5.0f, labelY - 2.0f);
        glVertex2f(windowWidth - 5.0f, labelY + labelHeight + 2.0f);
        glVertex2f(labelX - 5.0f, labelY + labelHeight + 2.0f);
    glEnd();
    
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    glBegin(GL_QUADS);
        glVertex2f(labelX, labelY);
        glVertex2f(labelX + labelWidth, labelY);
        glVertex2f(labelX + labelWidth, labelY + labelHeight);
        glVertex2f(labelX, labelY + labelHeight);
    glEnd();
    
    glDisable(GL_BLEND);
}
