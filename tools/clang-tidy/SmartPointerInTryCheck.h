#ifndef SAFETY_SMARTPOINTER_IN_TRY_CHECK_H
#define SAFETY_SMARTPOINTER_IN_TRY_CHECK_H

#include "clang-tidy/ClangTidyCheck.h"

namespace safety {

/**
 * SmartPointerInTryCheck - Enforces that safety::SmartPointer is only
 * constructed inside try/catch blocks
 * 
 * This check uses AST matchers to find all construction expressions of
 * safety::SmartPointer template class and verifies they are lexically
 * within a try statement.
 * 
 * Diagnostic message:
 *   "SmartPointer must be constructed inside a try/catch block.
 *    Use SafePointer or SafeResultPointer if exceptions are not handled."
 */
class SmartPointerInTryCheck : public clang::tidy::ClangTidyCheck {
public:
    SmartPointerInTryCheck(llvm::StringRef Name, 
                          clang::tidy::ClangTidyContext *Context)
        : ClangTidyCheck(Name, Context) {}
    
    void registerMatchers(clang::ast_matchers::MatchFinder *Finder) override;
    void check(const clang::ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace safety

#endif // SAFETY_SMARTPOINTER_IN_TRY_CHECK_H
