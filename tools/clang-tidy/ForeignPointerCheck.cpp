#include "ForeignPointerCheck.h"
#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tidy;

namespace safety {

void ForeignPointerCheck::registerMatchers(MatchFinder *Finder) {
    // Match std::unique_ptr with custom deleter (indicator of C API usage)
    // Look for: std::unique_ptr<Type, CustomDeleter>
    Finder->addMatcher(
        varDecl(
            hasType(
                qualType(
                    hasDeclaration(
                        classTemplateSpecializationDecl(
                            hasName("::std::unique_ptr"),
                            templateArgumentCountIs(2)
                        )
                    )
                )
            )
        ).bind("unique_ptr_var"),
        this
    );
}

void ForeignPointerCheck::check(const MatchFinder::MatchResult &Result) {
    const auto *VarD = Result.Nodes.getNodeAs<VarDecl>("unique_ptr_var");
    if (!VarD)
        return;
    
    // Get the type of the variable
    auto Type = VarD->getType();
    
    // Check if it's in a namespace that suggests it's a C API
    // (whisper_context, GLFW types, GPU contexts, etc.)
    std::string TypeStr = Type.getAsString();
    
    // Skip if it looks like a C++ type (has :: namespace or is in std::)
    if (TypeStr.find("::") != std::string::npos && 
        TypeStr.find("whisper") == std::string::npos &&
        TypeStr.find("GLFW") == std::string::npos &&
        TypeStr.find("ggml") == std::string::npos) {
        return;
    }
    
    // Check if the deleter is custom (not std::default_delete)
    if (TypeStr.find("default_delete") != std::string::npos) {
        // This is probably a regular C++ object, skip
        return;
    }
    
    // If we have a custom deleter, this is likely a C API handle
    if (TypeStr.find("unique_ptr<") != std::string::npos) {
        diag(VarD->getLocation(),
             "use safety::ForeignPointer instead of std::unique_ptr for C/FFI/GPU/OS handles")
            << FixItHint::CreateReplacement(
                VarD->getTypeSourceInfo()->getTypeLoc().getSourceRange(),
                "safety::ForeignPointer"
            );
        
        diag(VarD->getLocation(),
             "C/FFI/GPU/OS handles must use safety::ForeignPointer to keep ownership within the safety framework",
             DiagnosticIDs::Note);
    }
}

} // namespace safety
