# Project Reorganization - Status

## Target Structure

```
./
├─ src/                    # All source code
│  ├─ App/
│  │  ├─ main.cpp
│  │  ├─ AppHost.h/.cpp
│  │  ├─ version.h
│  │  ├─ stb_image.h
│  │  └─ DI/
│  └─ Services/
│     └─ [all services]/
├─ archive/                # Archived display/ files (*.old)
│  ├─ *.cpp.old
│  └─ *.h.old
├─ config/
│  ├─ scenes/              # Moved from ./scenes/
│  └─ audio_seed.txt
├─ Makefile                # ✅ Updated
├─ tests/
├─ wix/
├─ assets/
├─ include/
├─ lib/
├─ bin/
└─ docs/
```

## ✅ COMPLETED

**Makefile:**
- ✅ Updated SRCS: `App/` → `src/App/`, `Services/` → `src/Services/`
- ✅ Removed `display/*.cpp` from SRCS (replaced with `src/Services/*/*.cpp`)
- ✅ Updated CXXFLAGS: `-Isrc -Isrc/App -Isrc/Services`
- ✅ Updated version paths: `App/version.h` → `src/App/version.h`
- ✅ Updated test paths: `display/*.o` → `archive/*.o.old`

**Scene Paths:**
- ✅ `display/opening_scene.cpp`: `"scenes/"` → `"config/scenes/"`
- ✅ `display/render.cpp`: `"scenes/"` → `"config/scenes/"`
- ✅ `display/admin.cpp`: `"scenes/"` → `"config/scenes/"`
- ✅ `test/scene_test.cpp`: `"scenes/"` → `"config/scenes/"`

**Log Paths:**
- ✅ `display/scene_logger.cpp`: `"scene.log"` → `"logs/scene.log"`
- ✅ `display/scene_logger.cpp`: `"audio.log"` → `"logs/audio.log"`

## ⏳ MANUAL EXECUTION REQUIRED

**1. Run PowerShell Script to Move Files:**
```powershell
.\migrate-to-src.ps1
```

Or manually execute:
```bash
# Create directories
mkdir -p src archive config/scenes

# Move source directories
mv App src/App
mv Services src/Services

# Archive display/ files
mv display/*.cpp archive/ - rename to *.cpp.old
mv display/*.h archive/ - rename to *.h.old

# Move scenes
mv scenes/* config/scenes/
rmdir scenes
```

**2. After Files Are Moved, Update Includes:**
- `src/App/AppHost.cpp`: `"Services/..."` → `"../Services/..."`
- Other includes should work with `-Isrc` flag

**3. Verify Compilation:**
```bash
make clean
make build
```

## Include Path Strategy

With `CXXFLAGS += -Isrc`, includes work as:
- `src/App/main.cpp`: `#include "AppHost.h"` (same directory)
- `src/App/AppHost.cpp`: `#include "../Services/..."` (sibling directory)
- `src/Services/*/I*.h`: `#include "../App/DI/..."` (up to src/, then down)
- `src/App/DI/ServiceProvider.cpp`: `#include "../../Services/..."` (up 2, then down)

## Notes

- Makefile fully updated and ready for new structure
- Scene paths updated to `config/scenes/`
- Log paths updated to `logs/`
- Display files will be archived (not compiled)
- All source code will be under `src/`
- Root directory will be much cleaner
