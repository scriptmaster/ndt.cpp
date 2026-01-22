# Safety Infrastructure

This directory (`src/safety/`) contains reusable C++ safety infrastructure for enforcing memory-safety, exception-safety, and thread-safety across any C++ project.

## Overview

The safety infrastructure provides:

1. **Four RAII pointer types** for different resource ownership scenarios
2. **Scope markers** for distinguishing safe zones (`SafeScope`, `SafeBoundary`)
3. **Clang-tidy checks** for compile-time enforcement

## Pointer Types Summary

| Type | Throws | Usage |
|------|--------|-------|
| `safety::SafePointer` | ❌ | Everywhere (worker threads, realtime, loops) |
| `safety::SafeResultPointer` | ❌ | APIs, workers (with error messages) |
| `safety::SmartPointer` | ✅ | Startup / boundary only (inside try/catch) |
| `safety::ForeignPointer` | ❌ | C/FFI/GPU/OS handles (owned resources) |

## Application-Level Ownership Rules

1. **DO NOT** use raw owning pointers (`T*` with `new`/`delete`)
2. **Application-level ownership** must use:
   - `safety::SafePointer` - For general use
   - `safety::SafeResultPointer` - For APIs needing error info
3. **Throwing ownership** is ONLY allowed via:
   - `safety::SmartPointer` - Inside try/catch at startup boundaries
4. **C/FFI/GPU/OS handles** MUST use:
   - `safety::ForeignPointer` - For whisper, GLFW, GPU contexts, etc.
5. **C++ objects** (Scene, etc.) use:
   - `std::unique_ptr` - Standard C++ RAII

## Components

### `foreign_pointer.h` - ForeignPointer

**RAII wrapper for C/FFI/GPU/OS handles. Use for all foreign resources.**

**Features:**
- Automatic cleanup via custom deleter (RAII)
- Does NOT throw exceptions (safe for all contexts)
- Non-copyable (prevents double-free)
- Movable (allows transfer of ownership)
- Template-based with custom deleter support

**Example:**
```cpp
#include "safety/foreign_pointer.h"

// Custom deleter for C API resource
struct WhisperDeleter {
    void operator()(whisper_context* ctx) const noexcept {
        if (ctx) whisper_free(ctx);
    }
};

// Declare as member variable
safety::ForeignPointer<whisper_context*, WhisperDeleter> ctx_;

// Initialize
ctx_.reset(whisper_init_from_file("model.bin"));

// Use
if (ctx_) {
    whisper_full(ctx_.get(), params, data, size);
}
// Automatic cleanup when ForeignPointer is destroyed
```

### `safe_pointer.h` - SafePointer

**Non-throwing RAII wrapper for C-style resources. Use everywhere.**

**Features:**
- Automatic cleanup via destructor (RAII)
- Does NOT throw exceptions (safe for all contexts)
- Returns nullptr on failure (check with `if (ptr)`)
- Non-copyable (prevents double-free)
- Movable (allows transfer of ownership)

**Example:**
```cpp
#include "safety/safe_pointer.h"

// Worker thread or realtime code - no exceptions
void workerLoop() {
    safety::SafeScope scope;
    
    while (running) {
        safety::SafePointer<Resource, decltype(&create_resource), decltype(&destroy_resource)>
            ptr("config.txt", create_resource, destroy_resource);
        
        if (ptr) {
            // Use ptr.get()
            process(ptr.get());
        } else {
            // Handle failure without throwing
            logError("Failed to create resource");
        }
    }
}
```

### `safe_result_pointer.h` - SafeResultPointer

**Non-throwing RAII wrapper with error messages. Use in APIs and workers.**

**Features:**
- Automatic cleanup via destructor (RAII)
- Does NOT throw exceptions (safe for all contexts)
- Stores error message on failure
- Check success with `if (ptr)` or `has_error()`
- Get error with `ptr.error()`
- Non-copyable (prevents double-free)
- Movable (allows transfer of ownership)

**Example:**
```cpp
#include "safety/safe_result_pointer.h"

// API function returning status
bool loadModel(const char* path) {
    safety::SafeResultPointer<Model, decltype(&create_model), decltype(&destroy_model)>
        model(path, create_model, destroy_model);
    
    if (!model) {
        std::cerr << "Error: " << model.error() << std::endl;
        return false;
    }
    
    // Use model.get()
    return true;
}
```

### `smart_pointer.h` - SmartPointer

**Throwing RAII wrapper for C-style resources. Use ONLY in startup/init code inside try/catch.**

**Features:**
- Automatic cleanup via destructor (RAII)
- Throws exception on creation failure
- MUST be used inside try/catch blocks
- Non-copyable (prevents double-free)
- Movable (allows transfer of ownership)

**Example:**
```cpp
#include "safety/smart_pointer.h"

// Startup code - exceptions handled
void init() {
    safety::SafeBoundary boundary;
    
    try {
        safety::SmartPointer<whisper_context, decltype(&create_whisper), decltype(&destroy_whisper)>
            ctx("model.bin", create_whisper, destroy_whisper);
        
        // Use ctx.get() to access raw pointer
        whisper_full(ctx.get(), ...);
        
    } catch (const std::runtime_error& e) {
        std::cerr << "Failed to create resource: " << e.what() << std::endl;
        throw;  // Propagate to caller
    }
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

### Choosing the Right Pointer Type

**Use `ForeignPointer`:**
- ✅ C/FFI handles (whisper, ggml, etc.)
- ✅ GPU contexts and resources
- ✅ OS handles (file descriptors, etc.)
- ✅ Any foreign library resource requiring cleanup
- ✅ GLFW windows and contexts

**Use `SafePointer`:**
- ✅ Worker threads and realtime loops
- ✅ Any code in `SafeScope` contexts
- ✅ When you don't need error messages
- ✅ Performance-critical paths

**Use `SafeResultPointer`:**
- ✅ APIs that need to return error information
- ✅ Worker threads that need diagnostic messages
- ✅ When you want structured error handling without exceptions

**Use `SmartPointer`:**
- ✅ Startup and initialization code
- ✅ Inside `SafeBoundary` contexts
- ✅ Always inside try/catch blocks
- ✅ When exception-based error handling is acceptable

**Use `std::unique_ptr`:**
- ✅ C++ objects (Scene, custom classes, etc.)
- ✅ Standard C++ RAII management
- ✅ When not using C/FFI resources

### DO:
✅ Use `ForeignPointer` for ALL C/FFI/GPU/OS handles  
✅ Use `SafePointer` or `SafeResultPointer` in worker threads  
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
❌ Use `SmartPointer` in worker threads (use `SafePointer` instead)  
❌ Use `std::unique_ptr` for C/FFI handles (use `ForeignPointer` instead)  
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
