# Realtime STT Pipeline (Whisper)

This document lists the files involved in realtime silence detection and STT, and summarizes their responsibilities.

## Files to Review

- `src/Services/AudioCaptureService/AudioCapture.h`
  - Public capture API, device selection, callback counters, and silence flag.
- `src/Services/AudioCaptureService/AudioCapture.cpp`
  - WinMM device setup, stereo → mono downmix, buffer management, silence detection.
  - Thread safety for `capturedSamples` and atomic counters.
- `src/Services/AudioCaptureService/AudioCaptureService.h/.cpp`
  - Lifecycle service: start/stop capture with timeout guard.
- `src/Services/AudioProcessorService/IAudioProcessorService.h`
- `src/Services/AudioProcessorService/AudioProcessorService.h/.cpp`
  - Silence detection, speech chunking, and non‑blocking `PopSpeechChunk()`.
- `src/Services/AudioCaptureService/AudioWaveform.h/.cpp`
  - RMS waveform processing and `updateAudio()` used by main loop.
- `src/Services/WindowService/AppLoop.cpp`
  - Main-thread logging of capture counters and silence state.
- `src/Services/STTService/STTService.h/.cpp`
  - Background worker that triggers transcription on silence transitions.
  - Optional Whisper integration via `whisper.h` when available.
- `src/Services/AudioPlayerService/AudioGeneration.h/.cpp`
  - Audio generation lifecycle (seed‑based init / cleanup).
- `src/Services/AudioPlayerService/AudioSeed.h/.cpp`
  - Seed persistence (load/save).

## Data Flow Summary

1. `AudioCaptureService::Start()` initializes WinMM capture and starts double buffering.
2. `waveInProc()` downmixes stereo → mono, pushes samples into `capturedSamples`, forwards float samples to `AudioProcessorService`.
3. `AudioWaveform::updateAudio()` builds waveform bars for rendering.
4. `AppLoop` logs counters and silence state once per second (main thread).
5. `AudioProcessorService` emits completed speech chunks.
6. `STTService::Transcribe()` runs Whisper (if available) on each chunk from the main loop.

## Realtime STT Notes

- Whisper integration is compiled only when `whisper.h` is present.
- Model path is read from `WHISPER_MODEL` or defaults to `models/ggml-base.en.bin`.
- Audio is resampled to 16kHz before transcription.
