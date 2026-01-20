# Missing Implementations After Removing archive/display/ References

## WindowService.cpp needs:

1. **`createWindows()`** - Creates GLFW windows for all monitors
2. **`cleanupWindows()`** - Cleans up GLFW windows and textures
3. **`runMainLoop()`** - Main application loop (rendering, input, audio updates)
4. **Helper functions for runMainLoop():**
   - `shouldShutdownApplication()` - Check if shutdown requested
   - `processUserInput()` - Process keyboard input (Alt+F4)
   - `maintainWindowVisibility()` - Keep windows visible
   - `prepareWindowForRendering()` - From render.h
   - `handleDisplayState()` - From render.h
   - `renderContentForState()` - From render.h

## AudioService.cpp needs:

1. **`initializeSystems()`** - Initializes scene logger, network, audio generation, audio capture
2. **Audio functions:**
   - `initAudioGeneration()` - Initialize procedural audio
   - `loadAudioSeed()` - Load seed from config file
   - `getAudioSeed()` - Get current audio seed
   - `initAudioCapture()` - Initialize microphone capture
   - `startAudioCapture()` - Start capturing audio
   - `stopAudioCapture()` - Stop audio capture
   - `cleanupAudioCapture()` - Cleanup audio capture resources
   - `updateAudio()` - Update audio system per frame
   - `cleanupAudio()` - Cleanup audio generation
3. **Network functions:**
   - `initNetwork()` - Initialize WinSock2
   - `cleanupNetwork()` - Cleanup WinSock2
   - `sendAudioToWhisper()` - Send audio to STT server
4. **Logger functions:**
   - `initSceneLogger()` - Initialize scene.log and audio.log
   - `cleanupSceneLogger()` - Cleanup scene logger
   - `cleanupAudioLogger()` - Cleanup audio logger

## Status

All implementations from `archive/display/*.cpp` need to be moved into Services:
- `window.cpp` → `WindowService/WindowManager.cpp` (or private methods)
- `app.cpp` → Split between `WindowService/AppLoop.cpp` and `AudioService/AudioInitialization.cpp`
- `audio.cpp` → `AudioService/AudioSystem.cpp`
- `network.cpp` → `AudioService/NetworkManager.cpp`
- `scene_logger.cpp` → `AudioService/SceneLogger.cpp`

Since `archive/display/` files are not available, implementations must be recreated from scratch or retrieved from version control.
