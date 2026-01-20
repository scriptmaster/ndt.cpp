# Project Reorganization Plan

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
│     ├─ LoggingService/
│     ├─ WindowService/
│     └─ [other services]/
├─ archive/                # Archived display/ files (renamed to .old)
│  ├─ *.cpp.old
│  └─ *.h.old
├─ Makefile
├─ tests/
├─ wix/
├─ assets/
├─ config/
│  ├─ scenes/              # Moved from ./scenes/
│  └─ audio_seed.txt
├─ include/
├─ lib/
├─ bin/
└─ docs/
```

## Migration Steps

1. Create `src/` and `archive/` directories
2. Move `App/` → `src/App/`
3. Move `Services/` → `src/Services/`
4. Move `display/*.cpp` → `archive/*.cpp.old`
5. Move `display/*.h` → `archive/*.h.old`
6. Move `scenes/` → `config/scenes/`
7. Update all includes:
   - `App/` → `src/App/`
   - `Services/` → `src/Services/`
   - `scenes/` → `config/scenes/`
8. Update Makefile:
   - `SRCS`: `App/` → `src/App/`, `Services/` → `src/Services/`
   - `CXXFLAGS`: `-IApp` → `-Isrc/App`, `-IServices` → `-Isrc/Services`
   - Remove `display/` entries from SRCS
9. Update scene file paths in code
10. Verify compilation
