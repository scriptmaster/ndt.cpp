# Cursor Project Rules

## Vendor Code Protection (READ-ONLY)

The following directories are STABLE VENDOR CODE and MUST NEVER be modified:

- src/App/third_party/whisper/**
- src/App/third_party/whisper/ggml/**
- src/App/third_party/whisper/*.cpp
- any ggml or whisper source files

These files are read-only. They are not owned by this project.

## What IS allowed
- Modify Makefiles
- Modify CMakeLists.txt
- Modify build scripts (.sh, .ps1)
- Change compiler flags and definitions (-D flags)
- Add or remove source files from compilation
- Add or remove libraries during link
- Explain linker errors

## What is FORBIDDEN
- Editing whisper or ggml source files
- Commenting out backend registration symbols
- Removing CPU or GPU code paths
- "Simplifying" vendor code
- Auto-fixing by deleting code

## Enforcement
If a solution would require modifying vendor code:
- STOP
- Find alternate solutions or check the compiler flags and suggest alternatives.

Acknowledge these rules before suggesting any fix.
