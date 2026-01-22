#ifndef SAFETY_SMARTPOINTER_IN_TRY_CHECK_H
#define SAFETY_SMARTPOINTER_IN_TRY_CHECK_H

#include "clang-tidy/ClangTidyCheck.h"

namespace safety {

/**
 * SmartPointerInTryCheck - Enforces SmartPointer construction inside try/catch
 * 
 * This clang-tidy check ensures that safety::SmartPointer is only constructed
 * within a try/catch block, as SmartPointer throws exceptions on failure.
 * 
 * Diagnostic:
 *   "SmartPointer must be constructed inside a try/catch block.
 *    Use SafePointer or SafeResultPointer if exceptions are not handled."
 * 
 * Implementation:
 *   Uses AST matcher to find CXXConstructExpr for safety::SmartPointer
 *   Checks if the construction is lexically inside a CXXTryStmt
 *   Emits error if not inside try/catch
 */
class SmartPointerInTryCheck : public clang::tidy::ClangTidyCheck {
public:
    SmartPointerInTryCheck(llvm::StringRef Name, clang::tidy::ClangTidyContext* Context)
        : ClangTidyCheck(Name, Context) {}
    
    /**
     * Register AST matchers
     * Matches CXXConstructExpr for safety::SmartPointer template
     */
    void registerMatchers(clang::ast_matchers::MatchFinder* Finder) override;
    
    /**
     * Check matched AST nodes
     * Verifies SmartPointer construction is inside try/catch
     */
    void check(const clang::ast_matchers::MatchFinder::MatchResult& Result) override;
};

} // namespace safety

#endif // SAFETY_SMARTPOINTER_IN_TRY_CHECK_H
