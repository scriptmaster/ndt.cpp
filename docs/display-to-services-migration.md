# Display/ to Services/ Migration

## Status: IN PROGRESS

### ✅ COMPLETED

**LoggingService:**
- ✅ `display/logging.*` → `Services/LoggingService/Logging.*`

**WindowService:**
- ✅ `WindowData.h` - Shared data structures
- ✅ `TextureLoader.*` - Texture loading class
- ✅ `IWindowService.h` - Updated to use WindowData.h

### ⏳ REMAINING (10 files to migrate)

**WindowService (6 files):**
- ⏳ `display/window.cpp` (410 lines) → `WindowManager.*`
- ⏳ `display/render.*` → `Renderer.*`
- ⏳ `display/scene.*` (850+ lines) → `SceneManager.*`
- ⏳ `display/admin.*` → `AdminManager.*`
- ⏳ `display/app.*` → `AppLoop.*`
- ⏳ `display/opening_scene.*` (40 lines) → `OpeningSceneLoader.*`

**AudioService (3 files):**
- ⏳ `display/audio.*` (560+ lines) → `AudioSystem.*`
- ⏳ `display/network.*` (370+ lines) → `NetworkManager.*`
- ⏳ `display/scene_logger.*` (65 lines) → `SceneLogger.*`

## Strategy

Each `display/*` file becomes a class/namespace with one responsibility. Due to large file sizes, migration will:
1. Preserve exact functionality
2. Update includes to Services/* paths
3. Use namespace or class encapsulation
4. Update all dependent includes
5. Update Makefile
6. Delete display/ after verification

## Migration Pattern

```cpp
// Before (display/file.h):
void function();

// After (Services/ServiceName/Class.h):
namespace ClassName {
    void Function();
}
```
