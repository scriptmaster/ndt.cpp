# Clang-Tidy Custom Checks

This directory contains custom clang-tidy checks for the safety infrastructure.

## SmartPointerInTryCheck

A custom clang-tidy check that enforces `safety::SmartPointer` is only constructed inside try/catch blocks.

### Purpose

The `safety::SmartPointer` template is designed to throw exceptions when resource creation fails. To ensure all exceptions are handled properly, this check verifies that every `SmartPointer` construction is lexically within a try statement.

### Implementation

**Files:**
- `SmartPointerInTryCheck.h` - Header with class declaration
- `SmartPointerInTryCheck.cpp` - Implementation with AST matcher

**How It Works:**

1. **AST Matcher**: Finds all `CXXConstructExpr` nodes that construct `safety::SmartPointer`
2. **Parent Walk**: Walks up the AST from the construction site
3. **Try Detection**: Checks if any ancestor is a `CXXTryStmt`
4. **Diagnostic**: Emits error if no try/catch is found

### Diagnostic Message

```
SmartPointer must be constructed inside a try/catch block.
Use SafePointer or SafeResultPointer if exceptions are not handled.
```

### Example Violations

**Bad - No try/catch:**
```cpp
void initialize() {
    safety::SafeBoundary boundary;
    
    // ERROR: SmartPointer not in try/catch
    safety::SmartPointer<FILE*, ...> file("config.txt", creator, deleter);
}
```

**Good - Inside try/catch:**
```cpp
void initialize() {
    safety::SafeBoundary boundary;
    
    try {
        // OK: SmartPointer inside try/catch
        safety::SmartPointer<FILE*, ...> file("config.txt", creator, deleter);
        processFile(file.get());
    } catch (const std::exception& e) {
        logError(e.what());
    }
}
```

### Integration

To integrate this check into a clang-tidy build:

1. **Plugin Build** (if building as plugin):
   ```bash
   clang++ -shared -fPIC \
     -I/path/to/llvm/include \
     -I/path/to/clang/include \
     SmartPointerInTryCheck.cpp \
     -o libSafetyChecks.so
   ```

2. **Built-in Build** (if adding to clang-tidy source):
   - Add files to `clang-tools-extra/clang-tidy/safety/`
   - Register module in clang-tidy module list
   - Rebuild clang-tidy

3. **Configuration** (`.clang-tidy`):
   ```yaml
   Checks: 'safety-smartpointer-in-try'
   WarningsAsErrors: 'safety-smartpointer-in-try'
   ```

### Technical Details

**Namespace:** `safety`

**Base Class:** `clang::tidy::ClangTidyCheck`

**AST Matcher Pattern:**
```cpp
cxxConstructExpr(
    hasDeclaration(
        cxxConstructorDecl(
            ofClass(
                classTemplateSpecializationDecl(
                    hasName("safety::SmartPointer")
                )
            )
        )
    )
)
```

**Parent Traversal:**
Uses `ASTContext::getParents()` to walk up the AST hierarchy until either:
- A `CXXTryStmt` is found (valid)
- The top of the AST is reached (violation)

### Check Registration

To register with clang-tidy module system:

```cpp
// In module registration file
#include "SmartPointerInTryCheck.h"

class SafetyModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<safety::SmartPointerInTryCheck>(
        "safety-smartpointer-in-try");
  }
};

// Register the module
static ClangTidyModuleRegistry::Add<SafetyModule>
    X("safety-module", "Adds safety infrastructure checks.");
```

### Testing

Example test cases:

```cpp
// RUN: %check_clang_tidy %s safety-smartpointer-in-try %t

namespace safety {
template<typename T, typename C, typename D>
class SmartPointer {
public:
    SmartPointer(const std::string&, C, D);
};
}

void bad() {
    auto c = [](const char*) -> FILE* { return nullptr; };
    auto d = [](FILE*) {};
    
    // CHECK-MESSAGES: [[@LINE+1]]:5: warning: SmartPointer must be constructed inside a try/catch block
    safety::SmartPointer<FILE*, decltype(c), decltype(d)> p("test", c, d);
}

void good() {
    auto c = [](const char*) -> FILE* { return nullptr; };
    auto d = [](FILE*) {};
    
    try {
        // No warning - inside try/catch
        safety::SmartPointer<FILE*, decltype(c), decltype(d)> p("test", c, d);
    } catch (...) {
    }
}
```

### Limitations

1. **Lexical Scope Only**: Check is purely lexical - doesn't track exception propagation
2. **No Catch Validation**: Doesn't verify that catch blocks actually handle the exception
3. **Template Instantiation**: Checks construction sites, not template definitions

### Future Enhancements

Potential improvements:
- Check that catch blocks catch `std::exception` or appropriate types
- Verify exception specifications match SmartPointer's throws
- Add configuration options for allowed exception patterns
- Support whitelisting specific contexts
