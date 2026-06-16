#include "clad/Differentiator/ReverseModeVisitor.h"
#include "ConstantFolder.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
using namespace clang;
namespace clad{
    static void CloneCUDASharedAttr(const clang::VarDecl* OriginalVD, clang::VarDecl* VDClone) {
    if (OriginalVD->hasAttr<clang::CUDASharedAttr>()) {
        VDClone->addAttr(clang::CUDASharedAttr::CreateImplicit(OriginalVD->getASTContext()));
        }
    } 
    void ReverseModeVisitor::HandleCUDASharedMemoryDecl(
    const clang::VarDecl* VD, clang::VarDecl* VDDerived,
    llvm::SmallVectorImpl<clang::Stmt*>& memsetCalls) {
    
    CloneCUDASharedAttr(VD, VDDerived); 
    bool isDynamicSharedMem = VD->getType()->isIncompleteArrayType();

    // 2. Handle Zero-Initialization (Static vs Dynamic)
    if (!isDynamicSharedMem) {
      // Static Shared Memory zero-init
      llvm::SmallVector<Expr*, 1> args = {BuildDeclRef(VDDerived)};
      Stmt* initCall = GetCladZeroInit(args);
      if (initCall)
        memsetCalls.push_back(initCall);
    } else {
      // Dynamic Shared Memory: Automate zero-init purely via AST
      Expr* derivedRef = BuildDeclRef(VDDerived);
      Expr* zeroIdx =
          ConstantFolder::synthesizeLiteral(m_Context.IntTy, m_Context, 0);

      // Create the array subscript: _d_sharedMem1[0]
      Expr* arraySub = BuildArraySubscript(derivedRef, {zeroIdx});

      // Get the underlying type (e.g., int) so we assign the correct type of
      // zero
      QualType elemType = VDDerived->getType()->getPointeeType();

      // Create the assignment: _d_sharedMem1[0] = 0
      Expr* assignZero = BuildOp(BO_Assign, arraySub, getZeroInit(elemType));

      memsetCalls.push_back(assignZero);
    }


    }
}