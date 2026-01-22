#include "SmartPointerInTryCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang;
using namespace clang::ast_matchers;

namespace safety {

/**
 * Helper function to check if a statement is inside a try/catch block
 * Traverses parent nodes looking for CXXTryStmt
 */
static bool isInsideTryCatch(const Stmt* S, ASTContext* Context) {
    if (!S || !Context) {
        return false;
    }
    
    // Traverse parent nodes using ASTContext parent map
    auto Parents = Context->getParents(*S);
    while (!Parents.empty()) {
        // Check if current parent is a try statement
        const auto* TryStmt = Parents[0].get<CXXTryStmt>();
        if (TryStmt) {
            return true;
        }
        
        // Move to next parent
        const auto* ParentStmt = Parents[0].get<Stmt>();
        if (!ParentStmt) {
            break;
        }
        Parents = Context->getParents(*ParentStmt);
    }
    
    return false;
}

void SmartPointerInTryCheck::registerMatchers(MatchFinder* Finder) {
    // Match CXXConstructExpr for class template specialization of safety::SmartPointer
    Finder->addMatcher(
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
        ).bind("smartPointerConstruct"),
        this
    );
}

void SmartPointerInTryCheck::check(const MatchFinder::MatchResult& Result) {
    const auto* ConstructExpr = Result.Nodes.getNodeAs<CXXConstructExpr>("smartPointerConstruct");
    if (!ConstructExpr) {
        return;
    }
    
    // Check if construction is inside a try/catch block
    if (!isInsideTryCatch(ConstructExpr, Result.Context)) {
        diag(ConstructExpr->getBeginLoc(),
             "SmartPointer must be constructed inside a try/catch block. "
             "Use SafePointer or SafeResultPointer if exceptions are not handled.");
    }
}

} // namespace safety
