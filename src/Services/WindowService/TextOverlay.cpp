#include "TextOverlay.h"

#define STB_EASY_FONT_IMPLEMENTATION
#include "stb/stb_easy_font.h"

#ifdef _WIN32
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <algorithm>

namespace {
void drawTextLine(float x, float y, const std::string& line, float alpha) {
    if (line.empty()) {
        return;
    }
    char buffer[16384];
    const int num_quads = stb_easy_font_print(x, y, const_cast<char*>(line.c_str()), nullptr, buffer, sizeof(buffer));
    if (num_quads <= 0) {
        return;
    }
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}
}

void renderBottomCenterTextOverlay(const std::string& text, int fbWidth, int fbHeight) {
    if (text.empty() || fbWidth <= 0 || fbHeight <= 0) {
        return;
    }

    std::string clipped = text;
    const size_t kMaxChars = 200;
    if (clipped.size() > kMaxChars) {
        clipped = clipped.substr(0, kMaxChars - 3);
        clipped += "...";
    }

    const float margin = 6.0f;
    const float padding = 6.0f;
    const float textWidth = static_cast<float>(stb_easy_font_width(const_cast<char*>(clipped.c_str())));
    const float textHeight = static_cast<float>(stb_easy_font_height(const_cast<char*>(clipped.c_str())));

    float x = (static_cast<float>(fbWidth) - textWidth) * 0.5f - padding;
    float y = margin;
    x = std::max(0.0f, x);
    y = std::max(0.0f, y);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, fbWidth, fbHeight, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + textWidth + padding * 2.0f, y);
        glVertex2f(x + textWidth + padding * 2.0f, y + textHeight + padding * 2.0f);
        glVertex2f(x, y + textHeight + padding * 2.0f);
    glEnd();

    char buffer[16384];
    const float textX = x + padding;
    const float textY = y + padding;
    const int num_quads = stb_easy_font_print(textX, textY, const_cast<char*>(clipped.c_str()), nullptr, buffer, sizeof(buffer));
    if (num_quads > 0) {
        glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 16, buffer);
        glDrawArrays(GL_QUADS, 0, num_quads * 4);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    glDisable(GL_BLEND);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void renderServiceStatusOverlay(const std::vector<ServiceStatus>& statuses,
                                int fbWidth,
                                int fbHeight,
                                float startY,
                                double elapsedSeconds) {
    if (statuses.empty() || fbWidth <= 0 || fbHeight <= 0) {
        return;
    }

    const float lineHeight = 14.0f;
    float maxWidth = 0.0f;
    for (const auto& status : statuses) {
        const std::string line = std::string("[") + (status.started ? "x" : " ") + "] " + status.name;
        maxWidth = std::max(maxWidth, static_cast<float>(stb_easy_font_width(const_cast<char*>(line.c_str()))));
    }
    const std::string timerLine = "Startup: " + std::to_string(static_cast<int>(elapsedSeconds + 0.5)) + "s";
    maxWidth = std::max(maxWidth, static_cast<float>(stb_easy_font_width(const_cast<char*>(timerLine.c_str()))));

    float x = (static_cast<float>(fbWidth) - maxWidth) * 0.5f;
    float y = std::min(startY, static_cast<float>(fbHeight) - lineHeight);
    x = std::max(0.0f, x);
    y = std::max(0.0f, y);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, fbWidth, fbHeight, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float lineY = y;
    for (const auto& status : statuses) {
        const std::string line = std::string("[") + (status.started ? "x" : " ") + "] " + status.name;
        drawTextLine(x, lineY, line, status.started ? 1.0f : 0.25f);
        lineY += lineHeight;
    }

    lineY += 4.0f;
    drawTextLine(x, lineY, timerLine, 0.8f);

    glDisable(GL_BLEND);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
