#ifndef SAFETY_FOREIGN_POINTER_CHECK_H
#define SAFETY_FOREIGN_POINTER_CHECK_H

#include <clang-tidy/ClangTidy.h>
#include <clang-tidy/ClangTidyCheck.h>

namespace safety {

/**
 * ForeignPointerCheck - Enforces usage of safety::ForeignPointer for C/FFI handles
 * 
 * This check identifies std::unique_ptr usage with C API types and suggests
 * using safety::ForeignPointer instead.
 * 
 * Rationale:
 * - C/FFI/GPU/OS handles should use ForeignPointer for consistency
 * - Keeps all ownership semantics within the safety framework
 * - Makes it clear which resources are foreign vs C++ objects
 * 
 * Examples:
 * 
 * Bad:
 *   std::unique_ptr<whisper_context, WhisperDeleter> ctx;
 * 
 * Good:
 *   safety::ForeignPointer<whisper_context*, WhisperDeleter> ctx;
 */
class ForeignPointerCheck : public clang::tidy::ClangTidyCheck {
public:
    ForeignPointerCheck(llvm::StringRef Name, clang::tidy::ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context) {}
    
    void registerMatchers(clang::ast_matchers::MatchFinder *Finder) override;
    void check(const clang::ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace safety

#endif // SAFETY_FOREIGN_POINTER_CHECK_H
