# Audio Capture Architecture

Production-quality audio capture architecture for real-time STT (Speech-to-Text) systems.

## Architecture Overview

The audio capture pipeline is designed with clear separation of concerns:

```
PCM Samples (int16)
    ↓
RMSAnalyzer → RMS value (100ms window)
    ↓
NoiseCalibrator → Noise floor (300ms calibration)
    ↓
SpeechGate → isSpeaking() boolean (with hysteresis)
    ↓
AudioSegmentBuffer → PCM samples during speech
    ↓
[Future: Whisper STT Integration]
```

## Components

### 1. RMSAnalyzer
**Responsibility:** RMS calculation only

- Maintains fixed-time sliding window (default: 100ms)
- Thread-safe implementation
- Converts int16 PCM to normalized float internally
- Returns RMS only when window is full
- No historical bleed (window is reset when cleared)

**Key Methods:**
- `update(samples, numSamples)` - Add samples and get RMS
- `getRMS()` - Get current RMS value
- `reset()` - Clear window

### 2. NoiseCalibrator
**Responsibility:** Background noise floor estimation

- Collects RMS values during calibration window (default: 300ms)
- Computes average RMS as noise floor
- Locks value after calibration
- Prevents division by zero with minimum floor

**Key Methods:**
- `startCalibration()` - Begin calibration window
- `addRMSValue(rms)` - Add RMS sample (returns true when complete)
- `isCalibrated()` - Check if calibration complete
- `getNoiseFloor()` - Get calibrated noise floor

### 3. SpeechGate
**Responsibility:** Speech/silence detection with hysteresis

- Uses RMS and noise floor to detect speech
- Speech threshold: `noiseFloor * 2.5` (configurable)
- Silence threshold: `noiseFloor * 1.5` (hysteresis)
- Speech start hold: 200ms (prevents false starts)
- Speech end hold: 500ms (prevents choppy detection)
- Callbacks for speech start/end events

**Key Methods:**
- `update(rms, noiseFloor)` - Update state (returns true if changed)
- `isSpeaking()` - Get current state
- `setOnSpeechStart(callback)` - Set speech start hook
- `setOnSpeechEnd(callback)` - Set speech end hook

### 4. AudioSegmentBuffer
**Responsibility:** PCM buffering for STT

- Collects raw PCM samples (int16) only during speech
- Adds pre-padding (100ms before speech start)
- Adds post-padding (200ms after speech end)
- Provides contiguous buffers ready for Whisper
- Independent of RMS (only depends on speech gate state)

**Key Methods:**
- `addSamples(samples, numSamples, isSpeaking)` - Add samples
- `finalizeSegment()` - Mark segment as ready
- `hasSegment()` - Check if segment ready
- `consumeSegment()` - Get and clear segment

### 5. AudioCaptureOrchestrator
**Responsibility:** Pipeline coordination

- Receives PCM samples from platform capture
- Feeds samples through analysis pipeline
- Coordinates calibration, speech detection, and buffering
- Emits debug information
- Provides callbacks for segment ready events

**Key Methods:**
- `processSamples(samples, numSamples)` - Process new samples
- `start()` - Start processing pipeline
- `stop()` - Stop and finalize segments
- `setOnSegmentReady(callback)` - Set segment ready hook
- `setOnDebugInfo(callback)` - Set debug info hook

## Whisper Integration Point

The architecture is designed for easy Whisper integration. When a speech segment ends:

1. `AudioSegmentBuffer` finalizes the segment
2. `AudioCaptureOrchestrator` calls `onSegmentReady` callback
3. Callback receives `std::vector<int16_t>` with PCM samples
4. **TODO:** Send to Whisper STT service

Example integration:
```cpp
orchestrator.setOnSegmentReady([](const std::vector<int16_t>& segment) {
    // segment contains PCM samples ready for Whisper
    // whisperService->transcribe(segment, sampleRate);
});
```

## Thread Safety

- `RMSAnalyzer`: Thread-safe (mutex-protected)
- `NoiseCalibrator`: Thread-safe (mutex-protected)
- `SpeechGate`: Not thread-safe (single-threaded use)
- `AudioSegmentBuffer`: Not thread-safe (single-threaded use)
- `AudioCaptureOrchestrator`: Designed for single-threaded use

## Configuration

Default parameters (all configurable):

- **RMS Window:** 100ms
- **Calibration Duration:** 300ms
- **Speech Threshold:** 2.5x noise floor
- **Silence Threshold:** 1.5x noise floor
- **Speech Start Hold:** 200ms
- **Speech End Hold:** 500ms
- **Pre-padding:** 100ms
- **Post-padding:** 200ms

## Usage Example

```cpp
// Initialize
AudioCaptureOrchestrator orchestrator(44100);

// Set callbacks
orchestrator.setOnSegmentReady([](const std::vector<int16_t>& segment) {
    // Process segment with Whisper
});

// Start processing
orchestrator.start();

// Process samples (from platform capture)
while (capturing) {
    auto samples = capture.getSamples();
    orchestrator.processSamples(samples.data(), samples.size());
}

// Stop and finalize
orchestrator.stop();
```

## Design Principles

1. **Single Responsibility:** Each class has one clear purpose
2. **Composition over Inheritance:** Classes are composed, not inherited
3. **Thread Safety:** Where needed, mutex protection is used
4. **Future-Proof:** Clean interfaces ready for Whisper integration
5. **Production Quality:** Well-commented, readable, testable code
