#include <GL/GL.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <string>

#pragma comment(lib, "winmm.lib")

// Utility to normalize PCM samples and compute RMS
static float computeRMS(const std::deque<float>& window) {
    if (window.empty()) {
        return 0.0f;
    }
    float sumSq = 0.0f;
    for (float sample : window) {
        sumSq += sample * sample;
    }
    return std::sqrt(sumSq / window.size());
}

// Thread-safe buffer for samples collected by waveIn
struct CaptureBuffer {
    std::mutex mutex;
    std::vector<int16_t> samples;

    void append(const int16_t* data, int count) {
        std::lock_guard<std::mutex> lock(mutex);
        samples.insert(samples.end(), data, data + count);
    }

    std::vector<int16_t> fetch() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<int16_t> out;
        out.swap(samples);
        return out;
    }
};

struct PlaybackState {
    std::atomic<bool> done{false};
};

// Global capture state used by waveIn callback
static CaptureBuffer g_captureBuffer;
static std::atomic<bool> g_captureRunning{false};

// waveIn callback
static void CALLBACK AudioCaptureProc(HWAVEIN hwi, UINT msg, DWORD_PTR /*instance*/, DWORD_PTR param1, DWORD_PTR) {
    if (msg != WIM_DATA) {
        return;
    }
    WAVEHDR* header = reinterpret_cast<WAVEHDR*>(param1);
    if (!header || !(header->dwFlags & WHDR_DONE)) {
        return;
    }
    int samples = header->dwBytesRecorded / sizeof(int16_t);
    int16_t* data = reinterpret_cast<int16_t*>(header->lpData);
    g_captureBuffer.append(data, samples);
    if (g_captureRunning.load()) {
        waveInAddBuffer(hwi, header, sizeof(WAVEHDR));
    }
}

static GLuint g_fontBase = 0;

// Rendering helper
static void drawBars(GLFWwindow* window, const std::vector<float>& bars) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.0f, 0.4f, 1.0f);
    glBegin(GL_QUADS);
    float barWidth = static_cast<float>(width) / bars.size();
    for (size_t i = 0; i < bars.size(); ++i) {
        float rms = bars[i];
        float barHeight = std::min(rms * height * 4.0f, static_cast<float>(height));
        float x = width - (i + 1) * barWidth;
        float y = (height - barHeight) * 0.5f;
        glVertex2f(x, y);
        glVertex2f(x + barWidth * 0.8f, y);
        glVertex2f(x + barWidth * 0.8f, y + barHeight);
        glVertex2f(x, y + barHeight);
    }
    glEnd();
}

static void drawText(const std::wstring& text, float x, float y) {
    if (!g_fontBase) {
        return;
    }
    glColor3f(0.12f, 0.15f, 0.33f);
    glRasterPos2f(x, y);
    glListBase(g_fontBase);
    glCallLists(static_cast<GLsizei>(text.size()), GL_UNSIGNED_SHORT, text.c_str());
}

// Enumerate input devices
static std::vector<std::wstring> enumerateDevices() {
    std::vector<std::wstring> list;
    UINT count = waveInGetNumDevs();
    for (UINT i = 0; i < count; ++i) {
        WAVEINCAPSA caps{};
        if (waveInGetDevCapsA(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            std::wstring name(caps.szPname, caps.szPname + strlen(caps.szPname));
            list.push_back(name);
        }
    }
    return list;
}

static void setupFont(GLFWwindow* window) {
    HDC hdc = GetDC(glfwGetWin32Window(window));
    g_fontBase = glGenLists(256);
    SelectObject(hdc, GetStockObject(ANSI_VAR_FONT));
    wglUseFontBitmapsW(hdc, 0, 256, g_fontBase);
    ReleaseDC(glfwGetWin32Window(window), hdc);
}

static std::vector<std::wstring> g_deviceNames;

static void renderDeviceLabels(GLFWwindow* window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float y = height - 20.0f;
    for (const auto& label : g_deviceNames) {
        drawText(label, 10.0f, y);
        y -= 18.0f;
    }
}

int main() {
    constexpr int sampleRate = 44100;
    constexpr int rmsWindow = 1024;
    constexpr int barCount = 64;
    constexpr double captureDurationSeconds = 10.0;
    constexpr UINT deviceIndex = 0;

    enumerateDevices();

    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Audio UI Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    setupFont(window);
    glClearColor(0.95f, 0.95f, 0.98f, 1.0f);

    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = sampleRate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = format.nChannels * (format.wBitsPerSample / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    HWAVEIN waveIn = nullptr;
    WAVEHDR headers[2];
    const int bufferSamples = 44100;
    const int bufferBytes = bufferSamples * sizeof(int16_t);

    for (int i = 0; i < 2; ++i) {
        headers[i].lpData = static_cast<LPSTR>(GlobalAlloc(GPTR, bufferBytes));
        headers[i].dwBufferLength = bufferBytes;
    }

    if (waveInOpen(&waveIn, deviceIndex, &format, reinterpret_cast<DWORD_PTR>(&AudioCaptureProc), 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        std::cerr << "waveInOpen failed" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    for (int i = 0; i < 2; ++i) {
        waveInPrepareHeader(waveIn, &headers[i], sizeof(headers[i]));
        waveInAddBuffer(waveIn, &headers[i], sizeof(headers[i]));
    }
    g_captureRunning = true;
    waveInStart(waveIn);

    std::vector<float> bars(barCount, 0.0f);
    std::deque<float> rmsWindowDeque;
    auto startTime = std::chrono::steady_clock::now();
    bool running = true;

    g_deviceNames = enumerateDevices();

    while (running && !glfwWindowShouldClose(window)) {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - startTime).count();
        if (elapsed >= captureDurationSeconds) {
            break;
        }

        auto newSamples = g_captureBuffer.fetch();
        for (int16_t sample : newSamples) {
            float normalized = static_cast<float>(sample) / 32768.0f;
            rmsWindowDeque.push_back(normalized);
            if (static_cast<int>(rmsWindowDeque.size()) > rmsWindow) {
                rmsWindowDeque.pop_front();
            }
        }

        float rms = computeRMS(rmsWindowDeque);
        bars.insert(bars.begin(), rms);
        bars.pop_back();

        drawBars(window, bars);
        renderDeviceLabels(static_cast<float>(width), static_cast<float>(height));
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            running = false;
        }
        glfwSwapBuffers(window);
    }

    g_captureRunning = false;
    waveInStop(waveIn);
    waveInReset(waveIn);
    for (int i = 0; i < 2; ++i) {
        waveInUnprepareHeader(waveIn, &headers[i], sizeof(headers[i]));
        GlobalFree(headers[i].lpData);
    }
    waveInClose(waveIn);

    std::vector<int16_t> recordedSamples = g_captureBuffer.fetch();
    if (recordedSamples.empty()) {
        std::cerr << "No samples captured" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    HWAVEOUT waveOut = nullptr;
    WAVEHDR playHeader{};
    playHeader.lpData = reinterpret_cast<LPSTR>(recordedSamples.data());
    playHeader.dwBufferLength = static_cast<DWORD>(recordedSamples.size() * sizeof(int16_t));
    waveOutOpen(&waveOut, deviceIndex, &format, 0, 0, CALLBACK_NULL);
    waveOutPrepareHeader(waveOut, &playHeader, sizeof(playHeader));
    waveOutWrite(waveOut, &playHeader, sizeof(playHeader));

    size_t playbackIndex = 0;
    while (!glfwWindowShouldClose(window) && !(playHeader.dwFlags & WHDR_DONE)) {
        size_t chunk = std::min<size_t>(1024, recordedSamples.size() - playbackIndex);
        for (size_t i = 0; i < chunk; ++i) {
            float normalized = static_cast<float>(recordedSamples[playbackIndex + i]) / 32768.0f;
            rmsWindowDeque.push_back(normalized);
            if (static_cast<int>(rmsWindowDeque.size()) > rmsWindow) {
                rmsWindowDeque.pop_front();
            }
        }
        playbackIndex += chunk;

        float rms = computeRMS(rmsWindowDeque);
        bars.insert(bars.begin(), rms);
        bars.pop_back();

        drawBars(window, bars);
        renderDeviceLabels(static_cast<float>(width), static_cast<float>(height));
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }
        glfwSwapBuffers(window);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    waveOutReset(waveOut);
    waveOutUnprepareHeader(waveOut, &playHeader, sizeof(playHeader));
    waveOutClose(waveOut);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
