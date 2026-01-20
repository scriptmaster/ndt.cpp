# Migration Strategy

## Files Remaining (10 files, ~2,500+ lines)

Due to the large scope, this migration requires:
1. Creating 10 new class files
2. Updating 15+ include paths
3. Updating Makefile
4. Testing compilation

## Recommendation

Given the scope (~2,500+ lines across 10 files), the migration should proceed in phases:

**Phase 1 (Critical Path):**
- WindowManager (window.cpp) - Used by WindowService
- AppLoop (app.cpp) - Used by WindowService, AppHost
- AdminManager (admin.cpp) - Used by AppHost

**Phase 2 (Core Services):**
- AudioSystem, NetworkManager, SceneLogger - Used by AudioService

**Phase 3 (Rendering):**
- Renderer, SceneManager - Used by AppLoop

**Phase 4 (Completion):**
- OpeningSceneLoader - Used by AppLoop

Each class preserves exact functionality with updated includes and namespaces.
