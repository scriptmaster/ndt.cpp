# Audio Capture Service

This document describes the audio capture stack in `AudioCaptureService` and its supporting modules.

## Overview

- `AudioCaptureService` owns the lifecycle: initialize, start, stop, and cleanup.
- `AudioCapture` wraps the WinMM capture device (Windows only; stubs on non‑Windows).
- `AudioWaveform` converts captured samples into waveform bars for rendering.

## AudioCaptureService

**Files**
- `src/Services/AudioCaptureService/AudioCaptureService.h`
- `src/Services/AudioCaptureService/AudioCaptureService.cpp`

**Responsibilities**
- Start audio capture on service start.
- Stop and cleanup with a timeout guard to avoid shutdown hangs.
- Manage audio logger lifecycle.

**Functions**
- `Configure()`: No configuration (placeholder for future use).
- `Start()`:
  - Calls `initAudioLogger()`.
  - Calls `initAudioCapture(44100)` and starts capture with `startAudioCapture()`.
  - Non‑fatal: returns `true` even if capture init fails.
- `Stop()`:
  - Uses a timeout guard (3 seconds) to run:
    - `stopAudioCapture()`
    - `cleanupAudioCapture()`
  - Logs a timeout error if the worker does not complete.
  - Calls `cleanupAudioLogger()` and clears `initialized_`.

**Timeout Guard**
- Implemented with a worker thread and `std::condition_variable`.
- If the worker does not finish in time, the thread is detached and shutdown proceeds.

## AudioCapture (device layer)

**Files**
- `src/Services/AudioCaptureService/AudioCapture.h`
- `src/Services/AudioCaptureService/AudioCapture.cpp`

**Responsibilities**
- Manage WinMM capture device and buffers.
- Provide device name and captured samples.
- Feed samples into `AudioWaveform` via `updateAudioSamples`.

**Key Functions**
- `initAudioCapture(int sampleRate = 44100)`:
  - Initializes `WAVEFORMATEX`, enumerates devices, opens default device.
  - Allocates and prepares 2 input buffers (`waveHdr`).
- `startAudioCapture()`:
  - Queues buffers and starts capture (`waveInStart`).
- `stopAudioCapture()`:
  - Sets `audioCapturing = false` (prevents callback re‑queuing).
  - Calls `waveInStop`, `waveInReset`, unprepares headers.
- `cleanupAudioCapture()`:
  - Stops capture if needed, frees buffers, closes device handle.
- `listAllAudioDevices()`:
  - Enumerates and logs all input devices.
- `isAudioCapturing()` / `getCapturedAudioSamples()` / `getAudioDeviceName()`:
  - Status and data accessors.
- `getAudioCaptureCallbackCount()` / `getAudioCaptureZeroByteCount()`:
  - Atomic counters for main‑thread diagnostics.

**Notes**
- The callback `waveInProc` converts short samples to float and calls
  `updateAudioSamples()` for waveform processing.
- Non‑Windows builds use stub implementations.

## AudioWaveform (signal processing)

**Files**
- `src/Services/AudioCaptureService/AudioWaveform.h`
- `src/Services/AudioCaptureService/AudioWaveform.cpp`

**Responsibilities**
- Maintain a rolling sample buffer and RMS history.
- Convert RMS levels into bar heights for rendering.

**Key Functions**
- `setWaveformUpdateFPS(int fps)` / `getWaveformUpdateFPS()`:
  - Controls update rate (defaults to 10 fps).
- `updateAudioSamples(const float* samples, int numSamples)`:
  - Pushes samples into a circular buffer.
- `updateAudio(float deltaTime)`:
  - Runs at a throttled rate; computes RMS and bar heights.
  - Skips updates if capture is inactive (`isAudioCapturing()`).
- `getWaveformAmplitudes()`:
  - Returns the bar heights used by the renderer.

## Data Flow

1. `AudioCaptureService::Start()` → `initAudioCapture()` + `startAudioCapture()`
2. WinMM callback (`waveInProc`) → `updateAudioSamples()` + forwards float samples to `AudioProcessorService`
3. Main loop calls `updateAudio()` → bar history updated
4. Renderer reads `getWaveformAmplitudes()` and `getAudioDeviceName()`

## Common Failure Points

- `waveInReset` can block if the callback is still re‑queuing buffers.
  - Mitigated by setting `audioCapturing = false` before stop/reset.
- Shutdown hangs are mitigated by the timeout guard in `AudioCaptureService::Stop()`.
