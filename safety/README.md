# Safety Infrastructure

This directory contains reusable C++ safety infrastructure for enforcing memory-safety, exception-safety, and thread-safety across any C++ project.

## Overview

The safety infrastructure provides:

1. **RAII wrappers** for C-style resources (`SmartPointer`)
2. **Scope markers** for distinguishing safe zones (`SafeScope`, `SafeBoundary`)
3. **Clang-tidy checks** for compile-time enforcement

## Components

### `smart_pointer.h`

A generic RAII wrapper for C-style resources with custom create/destroy functions.

**Features:**
- Automatic cleanup via destructor (RAII)
- Throws exception on creation failure
- Non-copyable (prevents double-free)
- Movable (allows transfer of ownership)

**Example:**
```cpp
#include "safety/smart_pointer.h"

// Define wrapper functions for C API
whisper_context* create_whisper(const char* path) {
    return whisper_init_from_file(path);
}

void destroy_whisper(whisper_context* ctx) {
    whisper_free(ctx);
}

// Use SmartPointer in a try/catch block
try {
    safety::SmartPointer<whisper_context, decltype(&create_whisper), decltype(&destroy_whisper)>
        ctx("model.bin", create_whisper, destroy_whisper);
    
    // Use ctx.get() to access raw pointer
    whisper_full(ctx.get(), ...);
    
} catch (const std::runtime_error& e) {
    std::cerr << "Failed to create resource: " << e.what() << std::endl;
}
// Automatic cleanup when SmartPointer goes out of scope
```

### `safe_scope.h`

Scope markers for distinguishing exception boundaries.

#### `SafeScope`

Use at the beginning of worker threads, realtime loops, and other contexts where exceptions must NOT escape.

**Example:**
```cpp
void workerLoop() {
    safety::SafeScope scope;  // Worker thread - no exceptions allowed
    
    while (running) {
        // Worker code here
        // Do NOT use SmartPointer here (it throws)
        // Use SafePointer or SafeResultPointer instead
    }
}
```

#### `SafeBoundary`

Use at the beginning of startup, initialization, and boundary functions where exceptions ARE handled.

**Example:**
```cpp
void init() {
    safety::SafeBoundary boundary;  // Startup - exceptions handled
    
    try {
        safety::SmartPointer<...> resource(...);
        startSystem(resource.get());
    } catch (const std::exception& e) {
        logFatal(e.what());
        throw;  // Propagate to caller
    }
}
```

## Clang-Tidy Integration

The safety infrastructure includes a custom clang-tidy check to enforce that `SmartPointer` is only constructed inside try/catch blocks.

### Setup

The `.clang-tidy` configuration file is located in the project root with the following checks enabled:

```yaml
Checks:
  - safety-smartpointer-in-try  # Custom check for SmartPointer usage
  - cppcoreguidelines-owning-memory
  - cppcoreguidelines-no-malloc
  - bugprone-exception-escape
  - cert-err58-cpp
  - concurrency-mt-unsafe

WarningsAsErrors:
  - safety-smartpointer-in-try
  - cppcoreguidelines-owning-memory
  - bugprone-exception-escape
```

### Custom Check: `safety-smartpointer-in-try`

**What it does:** Ensures `safety::SmartPointer` is only constructed inside a try/catch block.

**Why:** SmartPointer throws exceptions on failure, so it must be inside a try/catch to handle errors properly.

**Diagnostic message:**
```
SmartPointer must be constructed inside a try/catch block.
Use SafePointer or SafeResultPointer if exceptions are not handled.
```

## Usage Guidelines

### DO:
✅ Use `SmartPointer` for C-style resources in startup/init code  
✅ Always construct `SmartPointer` inside try/catch blocks  
✅ Mark worker threads with `SafeScope`  
✅ Mark startup/init functions with `SafeBoundary`  
✅ Use `std::unique_ptr` for C++ objects (Scene, etc.)  
✅ Document any raw pointers required by third-party APIs

### DON'T:
❌ Use raw owning pointers (`T*` with `new`/`delete`)  
❌ Use `malloc`/`free` for memory allocation  
❌ Construct `SmartPointer` outside try/catch  
❌ Use `SmartPointer` in worker threads (use SafePointer instead)  
❌ Suppress clang-tidy warnings  
❌ Add project-specific names to safety infrastructure

## Building with Safety Checks

### Compile with clang-tidy:
```bash
clang-tidy src/**/*.cpp -- -std=c++17 -I. -Isrc
```

### Expected results:
- ✅ All code compiles without errors
- ✅ No clang-tidy warnings for safety checks
- ✅ Clean memory management via RAII

## Reusability

This safety infrastructure is **project-agnostic** and can be used in any C++ project:

1. Copy the `safety/` directory to your project
2. Copy the `.clang-tidy` configuration
3. Copy the `tools/clang-tidy/` directory (optional, for custom checks)
4. Add `-I<path-to-safety>` to your compiler flags
5. Start using `SmartPointer`, `SafeScope`, and `SafeBoundary`

## License

This safety infrastructure is part of the ndt.cpp project and follows the same license.
