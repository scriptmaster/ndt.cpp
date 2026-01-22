#include "SmartPointerInTryCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace safety {

void SmartPointerInTryCheck::registerMatchers(MatchFinder *Finder) {
    // Match construction of safety::SmartPointer
    // This matcher finds:
    // 1. Any CXXConstructExpr (constructor call)
    // 2. Where the type being constructed is a ClassTemplateSpecialization
    // 3. With template name "SmartPointer"
    // 4. In namespace "safety"
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
        ).bind("smartpointer_construct"),
        this
    );
}

void SmartPointerInTryCheck::check(const MatchFinder::MatchResult &Result) {
    const auto *ConstructExpr = Result.Nodes.getNodeAs<clang::CXXConstructExpr>("smartpointer_construct");
    if (!ConstructExpr) {
        return;
    }

    // Check if this construction is inside a try statement
    // Walk up the AST from the construct expression to find a try statement
    const clang::ASTContext *Context = Result.Context;
    const auto &Parents = Context->getParents(*ConstructExpr);
    
    bool insideTry = false;
    std::vector<clang::DynTypedNode> nodesToCheck = {clang::DynTypedNode::create(*ConstructExpr)};
    
    while (!nodesToCheck.empty() && !insideTry) {
        clang::DynTypedNode current = nodesToCheck.back();
        nodesToCheck.pop_back();
        
        // Check if current node is a CXXTryStmt
        if (current.get<clang::CXXTryStmt>()) {
            insideTry = true;
            break;
        }
        
        // Add parents to check
        const auto &currentParents = Context->getParents(current);
        for (const auto &parent : currentParents) {
            nodesToCheck.push_back(parent);
        }
    }
    
    // If not inside try block, emit diagnostic
    if (!insideTry) {
        diag(ConstructExpr->getBeginLoc(),
             "SmartPointer must be constructed inside a try/catch block. "
             "Use SafePointer or SafeResultPointer if exceptions are not handled.");
    }
}

} // namespace safety
