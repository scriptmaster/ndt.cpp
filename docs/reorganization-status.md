# Project Reorganization Status

## Target Structure

```
./
├─ src/                    # All source code ✅
│  ├─ App/
│  └─ Services/
├─ archive/                # Archived display/ files (*.old) ✅
├─ config/
│  └─ scenes/              # Moved from ./scenes/ ✅
├─ Makefile                # ✅
├─ tests/
├─ wix/
├─ assets/
├─ include/
├─ lib/
├─ bin/
└─ docs/
```

## Changes Required

### 1. Directory Moves
- [ ] `App/` → `src/App/`
- [ ] `Services/` → `src/Services/`
- [ ] `display/*.cpp` → `archive/*.cpp.old`
- [ ] `display/*.h` → `archive/*.h.old`
- [ ] `scenes/` → `config/scenes/`

### 2. Scene Path Updates
- [ ] `display/opening_scene.cpp`: `"scenes/"` → `"config/scenes/"`
- [ ] `display/render.cpp`: `"scenes/"` → `"config/scenes/"`
- [ ] `display/admin.cpp`: `"scenes/"` → `"config/scenes/"`
- [ ] `test/scene_test.cpp`: `"scenes/"` → `"config/scenes/"`

### 3. Include Path Updates
- [ ] All `App/` includes → `src/App/`
- [ ] All `Services/` includes → `src/Services/`
- [ ] Makefile: `-IApp` → `-Isrc/App`, `-IServices` → `-Isrc/Services`
- [ ] Makefile: `App/version.h` → `src/App/version.h`

### 4. Makefile Updates
- [ ] `SRCS`: `App/` → `src/App/`, `Services/` → `src/Services/`
- [ ] Remove `display/*.cpp` from SRCS
- [ ] `CXXFLAGS`: Update include paths
- [ ] Version file paths: `App/version.h` → `src/App/version.h`
