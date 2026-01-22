# Safety Infrastructure

This directory contains reusable C++ safety infrastructure for enforcing memory-safety, exception-safety, and thread-safety.

## Overview

The safety infrastructure provides:

1. **RAII-based resource management** via `SmartPointer<T, CreateFn, DestroyFn>`
2. **Scope markers** to distinguish between exception-allowed and no-throw zones
3. **Clang-tidy enforcement** to ensure rules are followed at compile-time

## Components

### 1. SmartPointer (`smart_pointer.h`)

A template class for RAII-based resource management with custom create/destroy functions.

**Key Features:**
- Template-based: `SmartPointer<T, CreateFn, DestroyFn>`
- Takes a string argument and calls `CreateFn(arg.c_str())`
- Throws `std::runtime_error` if creation fails (returns nullptr)
- Non-copyable, movable
- Destructor calls `DestroyFn(ptr)` with `noexcept` guarantee
- `get()` returns raw pointer for C API interop

**Usage Pattern:**

```cpp
#include "safety/smart_pointer.h"
#include "safety/safe_scope.h"
#include <cstdio>

void initialize() {
    safety::SafeBoundary boundary;  // Mark as exception boundary
    
    try {
        // Define create and destroy functions
        auto creator = [](const char* path) -> FILE* {
            return fopen(path, "r");
        };
        auto deleter = [](FILE* f) {
            if (f) fclose(f);
        };
        
        // Construct SmartPointer - must be inside try/catch
        safety::SmartPointer<FILE*, decltype(creator), decltype(deleter)>
            configFile("config.txt", creator, deleter);
        
        // Use the resource
        processFile(configFile.get());
        
    } catch (const std::exception& e) {
        logError(e.what());
    }
    // Resource automatically cleaned up here
}
```

### 2. Scope Markers (`safe_scope.h`)

Two marker types to distinguish between exception-allowed and no-throw contexts:

#### SafeBoundary

Use in startup/initialization functions or other boundary points where exceptions are acceptable.

```cpp
void startupFunction() {
    safety::SafeBoundary boundary;
    
    try {
        // Initialize resources - exceptions allowed
        auto resource = createResource();
    } catch (const std::exception& e) {
        logError(e.what());
    }
}
```

**Characteristics:**
- Exceptions are allowed
- SmartPointer construction is permitted (inside try/catch)
- Resource initialization can throw
- All exceptions must be caught at the boundary

#### SafeScope

Use in worker threads, real-time loops, or any code path that must not throw exceptions.

```cpp
void workerLoop() {
    safety::SafeScope scope;
    
    // No exceptions allowed here
    // No SmartPointer construction
    // Use error codes or logging instead
    while (running) {
        processData();
    }
}
```

**Characteristics:**
- No exceptions allowed
- No SmartPointer construction permitted
- Use error codes or logging for error handling
- No exception propagation

### 3. Clang-Tidy Enforcement

The safety infrastructure includes a custom clang-tidy check (`safety-smartpointer-in-try`) that enforces the rule:

**SmartPointer must only be constructed inside try/catch blocks**

This is configured in `.clang-tidy` at the project root.

## Clang-Tidy Configuration

The `.clang-tidy` file enables the following checks:

```yaml
Checks:
  safety-smartpointer-in-try         # Custom check for SmartPointer
  cppcoreguidelines-owning-memory    # Ban owning raw pointers
  cppcoreguidelines-no-malloc        # Ban malloc/free
  bugprone-exception-escape          # Detect exception leaks
  cert-err58-cpp                     # No throwing from constructors
  concurrency-mt-unsafe              # Thread-safety violations

WarningsAsErrors:
  safety-smartpointer-in-try         # Enforce SmartPointer in try/catch
  cppcoreguidelines-owning-memory    # Enforce no owning raw pointers
  bugprone-exception-escape          # Enforce exception safety
```

## Design Principles

### 1. Ban Owning Raw Pointers

Never use `T*` for ownership. Use:
- `SmartPointer<T, CreateFn, DestroyFn>` for custom resources (startup only)
- `std::unique_ptr<T>` for standard types
- `std::vector<T>` for dynamic arrays
- Stack allocation when possible

### 2. RAII Everywhere

All resources must be managed via RAII:
- Files: `SmartPointer<FILE*, ...>` or `std::unique_ptr<FILE, ...>`
- Memory: `std::vector`, `std::unique_ptr`, `std::string`
- Windows handles: `SmartPointer<HANDLE, ...>`
- OpenGL resources: Custom RAII wrappers

### 3. Exceptions Only at Boundaries

- **SafeBoundary zones**: Startup, initialization, configuration
  - Exceptions allowed, must be caught
  - SmartPointer construction permitted (in try/catch)
  
- **SafeScope zones**: Workers, loops, real-time code
  - No exceptions
  - No SmartPointer construction
  - Use error codes or logging

### 4. Enforced by Tooling

Safety rules are enforced by clang-tidy, not conventions:
- Custom checks ensure SmartPointer is only used in try/catch
- Standard checks ban malloc/free and owning raw pointers
- Warnings become errors to prevent violations

## Integration

### In Your Code

1. Include headers:
   ```cpp
   #include "safety/smart_pointer.h"
   #include "safety/safe_scope.h"
   ```

2. Mark scopes:
   ```cpp
   void main() {
       safety::SafeBoundary boundary;
       // ...
   }
   
   void workerThread() {
       safety::SafeScope scope;
       // ...
   }
   ```

3. Use SmartPointer for resources:
   ```cpp
   try {
       safety::SmartPointer<...> resource(...);
       // use resource
   } catch (const std::exception& e) {
       // handle error
   }
   ```

### In Your Build

Add to compiler flags:
```makefile
CXXFLAGS += -Isafety
```

Enable clang-tidy:
```bash
clang-tidy --config-file=.clang-tidy your_file.cpp
```

## Examples in This Project

- `main.cpp`: Uses `SafeBoundary` in main()
- `display/app.cpp`: 
  - `initializeSystems()` uses `SafeBoundary`
  - `runMainLoop()` uses `SafeScope`
- `display/audio.cpp`: Replaced malloc/free with `std::vector`
- `display/window.cpp`: Replaced `new`/`delete` with `std::unique_ptr`

## Why This Approach?

1. **Reusable**: Works in any C++ project, no project-specific names
2. **Enforced**: Clang-tidy catches violations at compile-time
3. **Explicit**: Scope markers make safety zones visible
4. **Flexible**: SmartPointer works with any C API via custom functions
5. **Safe**: RAII guarantees cleanup even during exceptions

## Future Enhancements

Potential additions (not implemented):
- `SafePointer<T>`: Non-owning pointer wrapper for SafeScope contexts
- `SafeResultPointer<T>`: Result-based error handling for SafeScope
- `SafeHandle<T>`: Platform-specific handle wrappers (Windows HANDLE, etc.)
