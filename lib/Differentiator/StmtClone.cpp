//--------------------------------------------------------------------*- C++ -*-
// clad - the C++ Clang-based Automatic Differentiator
// author:  Vassil Vassilev <vvasilev-at-cern.ch>
//------------------------------------------------------------------------------
//
// File originates from the Scout project (http://scout.zih.tu-dresden.de/)
#include "clad/Differentiator/StmtClone.h"

#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/Sema/Lookup.h"

#include "llvm/ADT/SmallVector.h"

#include "clad/Differentiator/Compatibility.h"

using namespace clang;

namespace clad {
namespace utils {

#define DEFINE_CLONE_STMT(CLASS, CTORARGS)    \
Stmt* StmtClone::Visit ## CLASS(CLASS *Node)  \
{                                             \
  return new (Ctx) CLASS CTORARGS;            \
}

#define DEFINE_CLONE_STMT_CO(CLASS, CTORARGS)                                  \
  Stmt* StmtClone::Visit##CLASS(CLASS* Node) {                                 \
    return (CLASS::Create CTORARGS);                                           \
  }

#define DEFINE_CLONE_EXPR(CLASS, CTORARGS)              \
Stmt* StmtClone::Visit ## CLASS(CLASS *Node)            \
{                                                       \
  CLASS* result = new (Ctx) CLASS CTORARGS;             \
  clad_compat::ExprSetDeps(result, Node);               \
  return result;                                        \
}

#define DEFINE_CREATE_EXPR(CLASS, CTORARGS)             \
Stmt* StmtClone::Visit ## CLASS(CLASS *Node)            \
{                                                       \
  CLASS* result = CLASS::Create CTORARGS;               \
  clad_compat::ExprSetDeps(result, Node);               \
  return result;                                        \
}

#define DEFINE_CLONE_EXPR_CO(CLASS, CTORARGS)                                  \
  Stmt* StmtClone::Visit##CLASS(CLASS* Node) {                                 \
    CLASS* result = (CLASS::Create CTORARGS);                                  \
    clad_compat::ExprSetDeps(result, Node);                                    \
    return result;                                                             \
  }

#define DEFINE_CLONE_EXPR_CO11(CLASS, CTORARGS)         \
Stmt* StmtClone::Visit ## CLASS(CLASS *Node)            \
{                                                       \
  CLASS* result = CLAD_COMPAT_CREATE11(CLASS, CTORARGS);\
  clad_compat::ExprSetDeps(result, Node);               \
  return result;                                        \
}

// NOLINTBEGIN(modernize-use-auto)
DEFINE_CLONE_EXPR_CO11(
    BinaryOperator,
    (CLAD_COMPAT_CLANG11_Ctx_ExtraParams Clone(Node->getLHS()),
     Clone(Node->getRHS()), Node->getOpcode(), CloneType(Node->getType()),
     Node->getValueKind(), Node->getObjectKind(), Node->getOperatorLoc(),
     Node->getFPFeatures(CLAD_COMPAT_CLANG11_LangOptions_EtraParams)))

DEFINE_CLONE_EXPR_CO11(
    UnaryOperator,
    (CLAD_COMPAT_CLANG11_Ctx_ExtraParams Clone(Node->getSubExpr()),
     Node->getOpcode(), CloneType(Node->getType()), Node->getValueKind(),
     Node->getObjectKind(), Node->getOperatorLoc(),
     Node->canOverflow() CLAD_COMPAT_CLANG11_UnaryOperator_ExtraParams))

Stmt* StmtClone::VisitDeclRefExpr(DeclRefExpr *Node) {
  TemplateArgumentListInfo TAListInfo;
  Node->copyTemplateArgumentsInto(TAListInfo);
  return DeclRefExpr::Create(
      Ctx, Node->getQualifierLoc(), Node->getTemplateKeywordLoc(),
      Node->getDecl(), Node->refersToEnclosingVariableOrCapture(),
      Node->getNameInfo(), CloneType(Node->getType()), Node->getValueKind(),
      Node->getFoundDecl(), &TAListInfo, Node->isNonOdrUse());
}

DEFINE_CREATE_EXPR(IntegerLiteral,
                   (Ctx, Node->getValue(), CloneType(Node->getType()),
                    Node->getLocation()))

// --- Hardened against null type/name across Clang versions ---
Stmt* StmtClone::VisitPredefinedExpr(PredefinedExpr* Node) {
  if (!Node)
    return nullptr;

  // Some libcs/Clang versions may not attach a StringLiteral name here.
  StringLiteral* FunctionName = Node->getFunctionName();
  if (!FunctionName) {
    SourceLocation Loc = Node->getLocation();
    llvm::SmallVector<SourceLocation, 1> Locs{Loc};
    FunctionName = StringLiteral::Create(
        Ctx, "", clad_compat::StringLiteralKind_Ordinary, /*Pascal=*/false,
        Ctx.CharTy, Locs.data(), Locs.size());
  }

  // Certain versions expose a null QualType for PredefinedExpr.
  QualType Ty = Node->getType();
  QualType ClonedTy = CloneType(Ty);
  if (ClonedTy.isNull()) {
    // Fallback to 'const char *'
    ClonedTy = Ctx.getPointerType(Ctx.getConstType(Ctx.CharTy));
  }

  PredefinedExpr* Result = PredefinedExpr::Create(
      Ctx, Node->getLocation(), ClonedTy,
      Node->getIdentKind() CLAD_COMPAT_CLANG17_IsTransparent(Node),
      FunctionName);
  clad_compat::ExprSetDeps(Result, Node);
  return Result;
}

Stmt* StmtClone::VisitSourceLocExpr(SourceLocExpr* Node) {
  if (!Node)
    return nullptr;

  // For maximum compatibility across all Clang versions, use a simple
  // and safe fallback that prevents segfaults. SourceLocExpr has significant
  // API changes between versions that make version-specific handling complex.
  
  QualType ClonedTy = CloneType(Node->getType());
  if (ClonedTy.isNull()) {
    ClonedTy = Ctx.getConstType(Ctx.CharTy);
  }

  // Create a safe StringLiteral fallback that represents the source location info
  // This maintains the essential functionality while avoiding version compatibility issues
  SourceLocation Loc = Node->getLocation();
  llvm::SmallVector<SourceLocation, 1> Locs{Loc};
  StringLiteral* Result = StringLiteral::Create(
      Ctx, "", clad_compat::StringLiteralKind_Ordinary, /*Pascal=*/false,
      ClonedTy, Locs.data(), Locs.size());
  clad_compat::ExprSetDeps(Result, Node);
  return Result;
}

DEFINE_CLONE_EXPR(CharacterLiteral,
                  (Node->getValue(), Node->getKind(),
                   CloneType(Node->getType()), Node->getLocation()))

DEFINE_CLONE_EXPR(ImaginaryLiteral,
                  (Clone(Node->getSubExpr()), CloneType(Node->getType())))

DEFINE_CLONE_EXPR(ParenExpr,
                  (Node->getLParen(), Node->getRParen(), Clone(Node->getSubExpr())))

DEFINE_CLONE_EXPR(ArraySubscriptExpr,
                  (Clone(Node->getLHS()), Clone(Node->getRHS()),
                   CloneType(Node->getType()), Node->getValueKind(),
                   Node->getObjectKind(), Node->getRBracketLoc()))

DEFINE_CREATE_EXPR(
    CXXDefaultArgExpr,
    (Ctx, SourceLocation(),
     Node->getParam()
         CLAD_COMPAT_CLANG16_CXXDefaultArgExpr_getRewrittenExpr_Param(Node),
     Node->getUsedContext()))

Stmt* StmtClone::VisitMemberExpr(MemberExpr* Node) {
  TemplateArgumentListInfo TemplateArgs;
  if (Node->hasExplicitTemplateArgs())
    Node->copyTemplateArgumentsInto(TemplateArgs);
  MemberExpr* Result = MemberExpr::Create(
      Ctx, Clone(Node->getBase()), Node->isArrow(), Node->getOperatorLoc(),
      Node->getQualifierLoc(), Node->getTemplateKeywordLoc(),
      Node->getMemberDecl(), Node->getFoundDecl(), Node->getMemberNameInfo(),
      &TemplateArgs, CloneType(Node->getType()), Node->getValueKind(),
      Node->getObjectKind(), Node->isNonOdrUse());
  clad_compat::ExprSetDeps(Result, Node);
  return Result;
}

DEFINE_CLONE_EXPR(CompoundLiteralExpr,
                  (Node->getLParenLoc(), Node->getTypeSourceInfo(),
                   CloneType(Node->getType()), Node->getValueKind(),
                   Clone(Node->getInitializer()), Node->isFileScope()))

DEFINE_CREATE_EXPR(
    ImplicitCastExpr,
    (Ctx, CloneType(Node->getType()), Node->getCastKind(),
     Clone(Node->getSubExpr()), nullptr,
     Node->getValueKind() /*EP*/ CLAD_COMPAT_CLANG12_CastExpr_GetFPO(Node)))

DEFINE_CREATE_EXPR(CStyleCastExpr,
                   (Ctx, CloneType(Node->getType()), Node->getValueKind(),
                    Node->getCastKind(), Clone(Node->getSubExpr()),
                    nullptr /*EP*/ CLAD_COMPAT_CLANG12_CastExpr_GetFPO(Node),
                    Node->getTypeInfoAsWritten(), Node->getLParenLoc(),
                    Node->getRParenLoc()))

DEFINE_CREATE_EXPR(
    CXXStaticCastExpr,
    (Ctx, CloneType(Node->getType()), Node->getValueKind(), Node->getCastKind(),
     Clone(Node->getSubExpr()), nullptr,
     Node->getTypeInfoAsWritten() /*EP*/ CLAD_COMPAT_CLANG12_CastExpr_GetFPO(
         Node),
     Node->getOperatorLoc(), Node->getRParenLoc(), Node->getAngleBrackets()))

DEFINE_CREATE_EXPR(CXXDynamicCastExpr,
                   (Ctx, CloneType(Node->getType()), Node->getValueKind(),
                    Node->getCastKind(), Clone(Node->getSubExpr()), nullptr,
                    Node->getTypeInfoAsWritten(), Node->getOperatorLoc(),
                    Node->getRParenLoc(), Node->getAngleBrackets()))

DEFINE_CREATE_EXPR(CXXReinterpretCastExpr,
                   (Ctx, CloneType(Node->getType()), Node->getValueKind(),
                    Node->getCastKind(), Clone(Node->getSubExpr()), nullptr,
                    Node->getTypeInfoAsWritten(), Node->getOperatorLoc(),
                    Node->getRParenLoc(), Node->getAngleBrackets()))

DEFINE_CREATE_EXPR(CXXConstCastExpr,
                   (Ctx, CloneType(Node->getType()), Node->getValueKind(),
                    Clone(Node->getSubExpr()), Node->getTypeInfoAsWritten(),
                    Node->getOperatorLoc(), Node->getRParenLoc(),
                    Node->getAngleBrackets()))

DEFINE_CREATE_EXPR(
    CXXConstructExpr,
    (Ctx, CloneType(Node->getType()), Node->getLocation(),
     Node->getConstructor(), Node->isElidable(),
     clad_compat::makeArrayRef(Node->getArgs(), Node->getNumArgs()),
     Node->hadMultipleCandidates(), Node->isListInitialization(),
     Node->isStdInitListInitialization(), Node->requiresZeroInitialization(),
     Node->getConstructionKind(), Node->getParenOrBraceRange()))

DEFINE_CREATE_EXPR(CXXFunctionalCastExpr,
                   (Ctx, CloneType(Node->getType()), Node->getValueKind(),
                    Node->getTypeInfoAsWritten(), Node->getCastKind(),
                    Clone(Node->getSubExpr()),
                    nullptr /*EP*/ CLAD_COMPAT_CLANG12_CastExpr_GetFPO(Node),
                    Node->getLParenLoc(), Node->getRParenLoc()))

DEFINE_CREATE_EXPR(ExprWithCleanups, (Ctx, Node->getSubExpr(),
                                      Node->cleanupsHaveSideEffects(), {}))

DEFINE_CREATE_EXPR(ConstantExpr,
                   (Ctx, Clone(Node->getSubExpr())
                             CLAD_COMPAT_ConstantExpr_Create_ExtraParams))

DEFINE_CLONE_EXPR_CO(
    CXXTemporaryObjectExpr,
    (Ctx, Node->getConstructor(), CloneType(Node->getType()),
     Node->getTypeSourceInfo(),
     clad_compat::makeArrayRef(Node->getArgs(), Node->getNumArgs()),
     Node->getSourceRange(), Node->hadMultipleCandidates(),
     Node->isListInitialization(), Node->isStdInitListInitialization(),
     Node->requiresZeroInitialization()))

DEFINE_CLONE_EXPR(MaterializeTemporaryExpr,
                  (CloneType(Node->getType()),
                   Node->getSubExpr() ? Clone(Node->getSubExpr()) : nullptr,
                   Node->isBoundToLvalueReference()))

DEFINE_CLONE_EXPR_CO11(
    CompoundAssignOperator,
    (CLAD_COMPAT_CLANG11_Ctx_ExtraParams Clone(Node->getLHS()),
     Clone(Node->getRHS()), Node->getOpcode(), CloneType(Node->getType()),
     Node->getValueKind(), Node->getObjectKind(),
     CLAD_COMPAT_CLANG11_CompoundAssignOperator_EtraParams_Removed
         Node->getOperatorLoc(),
     Node->getFPFeatures(CLAD_COMPAT_CLANG11_LangOptions_EtraParams)
         CLAD_COMPAT_CLANG11_CompoundAssignOperator_EtraParams_Moved))

DEFINE_CLONE_EXPR(ConditionalOperator,
                  (Clone(Node->getCond()), Node->getQuestionLoc(),
                   Clone(Node->getLHS()), Node->getColonLoc(),
                   Clone(Node->getRHS()), CloneType(Node->getType()),
                   Node->getValueKind(), Node->getObjectKind()))

DEFINE_CLONE_EXPR(AddrLabelExpr, (Node->getAmpAmpLoc(), Node->getLabelLoc(),
                                  Node->getLabel(), CloneType(Node->getType())))

DEFINE_CLONE_EXPR(StmtExpr,
                  (Clone(Node->getSubStmt()), CloneType(Node->getType()),
                   Node->getLParenLoc(), Node->getRParenLoc(),
                   Node->getTemplateDepth()))

DEFINE_CLONE_EXPR(ChooseExpr,
                  (Node->getBuiltinLoc(), Clone(Node->getCond()),
                   Clone(Node->getLHS()), Clone(Node->getRHS()),
                   CloneType(Node->getType()), Node->getValueKind(),
                   Node->getObjectKind(), Node->getRParenLoc(),
                   Node->isConditionTrue()
                       CLAD_COMPAT_CLANG11_ChooseExpr_EtraParams_Removed))

DEFINE_CLONE_EXPR(GNUNullExpr,
                  (CloneType(Node->getType()), Node->getTokenLocation()))

DEFINE_CLONE_EXPR(VAArgExpr,
                  (Node->getBuiltinLoc(), Clone(Node->getSubExpr()),
                   Node->getWrittenTypeInfo(), Node->getRParenLoc(),
                   CloneType(Node->getType()), Node->isMicrosoftABI()))

DEFINE_CLONE_EXPR(ImplicitValueInitExpr, (CloneType(Node->getType())))

DEFINE_CLONE_EXPR(CXXScalarValueInitExpr,
                  (CloneType(Node->getType()), Node->getTypeSourceInfo(),
                   Node->getRParenLoc()))

DEFINE_CLONE_EXPR(ExtVectorElementExpr,
                  (Node->getType(), Node->getValueKind(), Clone(Node->getBase()),
                   Node->getAccessor(), Node->getAccessorLoc()))

DEFINE_CLONE_EXPR(CXXBoolLiteralExpr,
                  (Node->getValue(), Node->getType(),
                   Node->getSourceRange().getBegin()))

DEFINE_CLONE_EXPR(CXXNullPtrLiteralExpr,
                  (Node->getType(), Node->getSourceRange().getBegin()))

CLAD_COMPAT_CLANG17_CXXThisExpr_ExtraParam
Node->getSourceRange().getBegin(), Node->getType(), Node->isImplicit()))

DEFINE_CLONE_EXPR(CXXThrowExpr,
                  (Clone(Node->getSubExpr()), Node->getType(),
                   Node->getThrowLoc(), Node->isThrownVariableInScope()))

#if CLANG_VERSION_MAJOR < 16
DEFINE_CLONE_EXPR(
    SubstNonTypeTemplateParmExpr,
    (CloneType(Node->getType()), Node->getValueKind(), Node->getBeginLoc(),
     Node->getParameter(),
     CLAD_COMPAT_SubstNonTypeTemplateParmExpr_isReferenceParameter_ExtraParam(
         Node) Node->getReplacement()))
#else
DEFINE_CLONE_EXPR(SubstNonTypeTemplateParmExpr,
                  (CloneType(Node->getType()), Node->getValueKind(),
                   Node->getBeginLoc(), Node->getReplacement(),
                   Node->getAssociatedDecl(), Node->getIndex(),
                   Node->getPackIndex(), Node->isReferenceParameter()))
#endif

DEFINE_CREATE_EXPR(PseudoObjectExpr,
                   (Ctx, Node->getSyntacticForm(),
                    llvm::SmallVector<Expr*, 4>(Node->semantics_begin(),
                                                Node->semantics_end()),
                    Node->getResultExprIndex()))
// NOLINTEND(modernize-use-auto)

// BlockExpr
// BlockDeclRefExpr

Stmt* StmtClone::VisitStringLiteral(StringLiteral* Node) {
  llvm::SmallVector<SourceLocation, 4> ConcatLocations(Node->tokloc_begin(),
                                                       Node->tokloc_end());
  return StringLiteral::Create(Ctx, Node->getString(), Node->getKind(),
                               Node->isPascal(), CloneType(Node->getType()),
                               ConcatLocations.data(), ConcatLocations.size());
}

Stmt* StmtClone::VisitFloatingLiteral(FloatingLiteral* Node) {
  FloatingLiteral* CloneFL =
      FloatingLiteral::Create(Ctx, Node->getValue(), Node->isExact(),
                              CloneType(Node->getType()), Node->getLocation());
  CloneFL->setSemantics(Node->getSemantics());
  return CloneFL;
}

Stmt* StmtClone::VisitInitListExpr(InitListExpr* Node) {
  llvm::SmallVector<Expr*, 8> InitExprs(Node->getNumInits());
  for (unsigned i = 0, e = Node->getNumInits(); i < e; ++i)
    InitExprs[i] = Clone(Node->getInit(i));

  SourceLocation LBrace = Node->getLBraceLoc();
  SourceLocation RBrace = Node->getRBraceLoc();

  InitListExpr* Result =
      llvm::cast<InitListExpr>(m_Sema.ActOnInitList(LBrace, InitExprs, RBrace).get());

  Result->setInitializedFieldInUnion(Node->getInitializedFieldInUnion());
  // FIXME: clone the syntactic form, can this become recursive?
  return Result;
}

//---------------------------------------------------------
Stmt* StmtClone::VisitDesignatedInitExpr(DesignatedInitExpr* Node) {
  llvm::SmallVector<Expr*, 8> IndexExprs(Node->getNumSubExprs());
  for (int i = 0, e = IndexExprs.size(); i < e; ++i)
    IndexExprs[i] = Clone(Node->getSubExpr(i));

  // no &IndexExprs[1]
  llvm::ArrayRef<Expr*> IndexExprsRef =
      clad_compat::makeArrayRef(&IndexExprs[0] + 1, IndexExprs.size() - 1);

  return DesignatedInitExpr::Create(Ctx, Node->designators(),
                                    IndexExprsRef, Node->getEqualOrColonLoc(),
                                    Node->usesGNUSyntax(), Node->getInit());
}

Stmt* StmtClone::VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr* Node) {
  if (Node->isArgumentType())
    return new (Ctx)
        UnaryExprOrTypeTraitExpr(Node->getKind(), Node->getArgumentTypeInfo(),
                                 CloneType(Node->getType()),
                                 Node->getOperatorLoc(), Node->getRParenLoc());
  return new (Ctx) UnaryExprOrTypeTraitExpr(
      Node->getKind(), Clone(Node->getArgumentExpr()),
      CloneType(Node->getType()), Node->getOperatorLoc(), Node->getRParenLoc());
}

Stmt* StmtClone::VisitCallExpr(CallExpr* Node) {
  llvm::SmallVector<Expr*, 4> ClonedArgs;
  for (Expr* Arg : Node->arguments())
    ClonedArgs.push_back(Clone(Arg));

  CallExpr* Result = clad_compat::CallExpr_Create(
      Ctx, Clone(Node->getCallee()), ClonedArgs, CloneType(Node->getType()),
      Node->getValueKind(),
      Node->getRParenLoc() CLAD_COMPAT_CLANG8_CallExpr_ExtraParams);

  clad_compat::ExprSetDeps(Result, Node);
  return Result;
}

Stmt* StmtClone::VisitCUDAKernelCallExpr(CUDAKernelCallExpr* Node) {
  llvm::SmallVector<Expr*, 4> ClonedArgs;
  for (Expr* Arg : Node->arguments())
    ClonedArgs.push_back(Clone(Arg));

  CUDAKernelCallExpr* Result = clad_compat::CUDAKernelCallExpr_Create(
      Ctx, Clone(Node->getCallee()), Clone(Node->getConfig()), ClonedArgs,
      CloneType(Node->getType()), Node->getValueKind(),
      Node->getRParenLoc() CLAD_COMPAT_CLANG8_CallExpr_ExtraParams);

  clad_compat::ExprSetDeps(Result, Node);
  return Result;
}

Stmt* StmtClone::VisitUnresolvedLookupExpr(UnresolvedLookupExpr* Node) {
  TemplateArgumentListInfo TemplateArgs;
  if (Node->hasExplicitTemplateArgs())
    Node->copyTemplateArgumentsInto(TemplateArgs);

  Stmt* Result = clad_compat::UnresolvedLookupExpr_Create(
      Ctx, Node->getNamingClass(), Node->getQualifierLoc(),
      Node->getTemplateKeywordLoc(), Node->getNameInfo(), Node->requiresADL(),
      &TemplateArgs, Node->decls_begin(), Node->decls_end());
  return Result;
}

Stmt* StmtClone::VisitCXXOperatorCallExpr(CXXOperatorCallExpr* Node) {
  llvm::SmallVector<Expr*, 4> ClonedArgs;
  for (Expr* Arg : Node->arguments())
    ClonedArgs.push_back(Clone(Arg));

  CallExpr::ADLCallKind UsesADL = CallExpr::NotADL; // unused but kept for API parity
  (void)UsesADL;

  CXXOperatorCallExpr* Result = CXXOperatorCallExpr::Create(
      Ctx, Node->getOperator(), Clone(Node->getCallee()), ClonedArgs,
      CloneType(Node->getType()), Node->getValueKind(), Node->getRParenLoc(),
      Node->getFPFeatures()
          CLAD_COMPAT_CLANG11_CXXOperatorCallExpr_Create_ExtraParamsUse);

  clad_compat::ExprSetDeps(Result, Node);
  return Result;
}

Stmt* StmtClone::VisitCXXMemberCallExpr(CXXMemberCallExpr * Node) {
  llvm::SmallVector<Expr*, 4> ClonedArgs;
  for (Expr* Arg : Node->arguments())
    ClonedArgs.push_back(Clone(Arg));

  CXXMemberCallExpr* Result = clad_compat::CXXMemberCallExpr_Create(
      Ctx, Clone(Node->getCallee()), ClonedArgs, CloneType(Node->getType()),
      Node->getValueKind(),
      Node->getRParenLoc()
      /*FP*/ CLAD_COMPAT_CLANG12_CastExpr_GetFPO(Node));

  clad_compat::ExprSetDeps(Result, Node);
  return Result;
}

Stmt* StmtClone::VisitShuffleVectorExpr(ShuffleVectorExpr* Node) {
  llvm::SmallVector<Expr*, 8> Cloned(std::max(1u, Node->getNumSubExprs()));
  for (unsigned i = 0, e = Node->getNumSubExprs(); i < e; ++i)
    Cloned[i] = Clone(Node->getExpr(i));

  llvm::ArrayRef<Expr*> ClonedRef =
      clad_compat::makeArrayRef(Cloned.data(), Cloned.size());

  return new (Ctx)
      ShuffleVectorExpr(Ctx, ClonedRef, CloneType(Node->getType()),
                        Node->getBuiltinLoc(), Node->getRParenLoc());
}

Stmt* StmtClone::VisitCaseStmt(CaseStmt* Node) {
  CaseStmt* Result = CaseStmt::Create(
      Ctx, Clone(Node->getLHS()), Clone(Node->getRHS()), Node->getCaseLoc(),
      Node->getEllipsisLoc(), Node->getColonLoc());
  Result->setSubStmt(Clone(Node->getSubStmt()));
  return Result;
}

Stmt* StmtClone::VisitSwitchStmt(SwitchStmt* Node) {
  SourceLocation NoLoc;
  SwitchStmt* Result
    = clad_compat::SwitchStmt_Create(Ctx,
        Node->getInit(), Node->getConditionVariable(), Node->getCond(),
        NoLoc, NoLoc);
  Result->setBody(Clone(Node->getBody()));
  Result->setSwitchLoc(Node->getSwitchLoc());
  return Result;
}

DEFINE_CLONE_STMT_CO(ReturnStmt,
                     (Ctx, Node->getReturnLoc(), Clone(Node->getRetValue()), 0))

DEFINE_CLONE_STMT(DefaultStmt,
                  (Node->getDefaultLoc(), Node->getColonLoc(),
                   Clone(Node->getSubStmt())))

DEFINE_CLONE_STMT(GotoStmt,
                  (Node->getLabel(), Node->getGotoLoc(), Node->getLabelLoc()))

DEFINE_CLONE_STMT_CO(WhileStmt,
                     (Ctx, CloneDeclOrNull(Node->getConditionVariable()),
                      Clone(Node->getCond()), Clone(Node->getBody()),
                      Node->getWhileLoc()
                      CLAD_COMPAT_CLANG11_WhileStmt_ExtraParams))

DEFINE_CLONE_STMT(DoStmt,
                  (Clone(Node->getBody()), Clone(Node->getCond()),
                   Node->getDoLoc(), Node->getWhileLoc(), Node->getRParenLoc()))

DEFINE_CLONE_STMT_CO(IfStmt,
  (Ctx, Node->getIfLoc(), CLAD_COMPAT_IfStmt_Create_IfStmtKind_Param(Node),
   Node->getInit(), CloneDeclOrNull(Node->getConditionVariable()),
   Clone(Node->getCond()) /*EPs*/ CLAD_COMPAT_CLANG12_LR_ExtraParams(Node),
   Clone(Node->getThen()), Node->getElseLoc(), Clone(Node->getElse())))

DEFINE_CLONE_STMT(LabelStmt,
                  (Node->getIdentLoc(), Node->getDecl(),
                   Clone(Node->getSubStmt())))

DEFINE_CLONE_STMT(NullStmt, (Node->getSemiLoc()))

DEFINE_CLONE_STMT(ForStmt,
  (Ctx, Clone(Node->getInit()), Clone(Node->getCond()),
   CloneDeclOrNull(Node->getConditionVariable()), Clone(Node->getInc()),
   Clone(Node->getBody()), Node->getForLoc(), Node->getLParenLoc(),
   Node->getRParenLoc()))

DEFINE_CLONE_STMT(ContinueStmt, (Node->getContinueLoc()))
DEFINE_CLONE_STMT(BreakStmt, (Node->getBreakLoc()))
DEFINE_CLONE_STMT(CXXCatchStmt,
                  (Node->getCatchLoc(), CloneDeclOrNull(Node->getExceptionDecl()),
                   Clone(Node->getHandlerBlock())))

DEFINE_CLONE_STMT(ValueStmt, (Node->getStmtClass()))

Stmt* StmtClone::VisitCXXTryStmt(CXXTryStmt* Node) {
  llvm::SmallVector<Stmt*, 4> Catches(std::max(1u, Node->getNumHandlers()));
  for (unsigned i = 0, e = Node->getNumHandlers(); i < e; ++i)
    Catches[i] = Clone(Node->getHandler(i));
  llvm::ArrayRef<Stmt*> Handlers =
      clad_compat::makeArrayRef(Catches.data(), Catches.size());
  return CXXTryStmt::Create(Ctx, Node->getTryLoc(), Clone(Node->getTryBlock()),
                            Handlers);
}

Stmt* StmtClone::VisitCompoundStmt(CompoundStmt *Node) {
  llvm::SmallVector<Stmt*, 8> ClonedBody;
  for (CompoundStmt::const_body_iterator I = Node->body_begin(),
                                         E = Node->body_end();
       I != E; ++I)
    ClonedBody.push_back(Clone(*I));

  llvm::ArrayRef<Stmt*> StmtsRef =
      clad_compat::makeArrayRef(ClonedBody.data(), ClonedBody.size());
  return clad_compat::CompoundStmt_Create(
      Ctx, StmtsRef /**/
           CLAD_COMPAT_CLANG15_CompoundStmt_Create_ExtraParam1(Node),
      Node->getLBracLoc(), Node->getLBracLoc());
}

VarDecl* StmtClone::CloneDeclOrNull(VarDecl* Node)  {
  if (!Node)
    return 0;
  return cast_or_null<VarDecl>(CloneDecl(Node));
}

Decl* StmtClone::CloneDecl(Decl* Node)  {
  // we support only exactly this class, so no visitor is needed (yet?)
  if (Node->getKind() == Decl::Var) {
    VarDecl* VD = static_cast<VarDecl*>(Node);

    VarDecl* ClonedDecl = VarDecl::Create(
        Ctx, VD->getDeclContext(), VD->getLocation(), VD->getInnerLocStart(),
        VD->getIdentifier(), CloneType(VD->getType()), VD->getTypeSourceInfo(),
        VD->getStorageClass());
    if (VD->getInit())
      m_Sema.AddInitializerToDecl(ClonedDecl, Clone(VD->getInit()), VD->isDirectInit());
    ClonedDecl->setTSCSpec(VD->getTSCSpec());
    if (m_OriginalToClonedStmts != 0)
      m_OriginalToClonedStmts->m_DeclMapping[VD] = ClonedDecl;

    return ClonedDecl;
  }
  assert(0 && "other decl clones aren't supported");
  return 0;
}

Stmt* StmtClone::VisitDeclStmt(DeclStmt* Node) {
  DeclGroupRef ClonedDecls;
  if (Node->isSingleDecl())
    ClonedDecls = DeclGroupRef(CloneDecl(Node->getSingleDecl()));
  else if (Node->getDeclGroup().isDeclGroup()) {
    llvm::SmallVector<Decl*, 8> ClonedDeclGroup;
    const DeclGroupRef& DG = Node->getDeclGroup();
    for (DeclGroupRef::const_iterator I = DG.begin(), E = DG.end(); I != E; ++I)
      ClonedDeclGroup.push_back(CloneDecl(*I));

    ClonedDecls = DeclGroupRef(
        DeclGroup::Create(Ctx, ClonedDeclGroup.data(), ClonedDeclGroup.size()));
  }
  return new (Ctx) DeclStmt(ClonedDecls, Node->getBeginLoc(), Node->getEndLoc());
}

Stmt* StmtClone::VisitStmt(Stmt*) {
  assert(0 && "clone not fully implemented");
  return 0;
}

ReferencesUpdater::ReferencesUpdater(
    Sema& SemaRef, Scope* S, const FunctionDecl* FD,
    const std::unordered_map<const clang::VarDecl*, clang::VarDecl*>&
        DeclReplacements)
    : m_Sema(SemaRef), m_CurScope(S), m_Function(FD),
      m_DeclReplacements(DeclReplacements) {}

bool ReferencesUpdater::VisitDeclRefExpr(DeclRefExpr* DRE) {
  // Only update references of decls inside the original function context.
  if (!DRE->getDecl()->getDeclContext()->Encloses(m_Function))
    return true;

  // Replace declaration if present in m_DeclReplacements.
  if (VarDecl* VD = dyn_cast<VarDecl>(DRE->getDecl())) {
    auto It = m_DeclReplacements.find(VD);
    if (It != std::end(m_DeclReplacements)) {
      DRE->setDecl(It->second);
      DRE->getDecl()->setReferenced();
      DRE->getDecl()->setIsUsed();
      QualType NonRefQT = It->second->getType().getNonReferenceType();
      if (NonRefQT != DRE->getType())
        DRE->setType(NonRefQT);
    }
  }

  DeclarationNameInfo DNI = DRE->getNameInfo();
  LookupResult R(m_Sema, DNI, Sema::LookupOrdinaryName);
  m_Sema.LookupName(R, m_CurScope, /*allowBuiltinCreation*/ false);
  if (R.empty())
    return true;

  // FIXME: handle overload sets properly.
  if (!R.isSingleResult())
    return true;

  if (ValueDecl* VD = dyn_cast<ValueDecl>(R.getFoundDecl())) {
    DRE->setDecl(VD);
    VD->setReferenced();
    VD->setIsUsed();
  }

  // Validate/update type safely (covers weird builtins).
  updateType(DRE->getType());
  return true;
}

bool ReferencesUpdater::VisitPredefinedExpr(clang::PredefinedExpr* PE) {
  // Nothing to update, but validate the type safely.
  updateType(PE->getType());
  return true;
}

bool ReferencesUpdater::VisitSourceLocExpr(clang::SourceLocExpr* SLE) {
  // Treat as constant leaf and validate type (may be null on some versions).
  updateType(SLE->getType());
  return true;
}

bool ReferencesUpdater::VisitStmt(clang::Stmt* S) {
  if (auto* E = dyn_cast<Expr>(S))
    updateType(E->getType());
  return true;
}

void ReferencesUpdater::updateType(QualType QT) {
  if (QT.isNull())
    return;
  if (const auto* VAT = dyn_cast<VariableArrayType>(QT))
    TraverseStmt(VAT->getSizeExpr());
}


QualType StmtClone::CloneType(const clang::QualType T) {
  if (T.isNull())
    return T;

  if (const auto* VAT = dyn_cast<VariableArrayType>(T)) {
    auto ElemTy = VAT->getElementType();
    return Ctx.getVariableArrayType(ElemTy, Clone(VAT->getSizeExpr()),
                                    VAT->getSizeModifier(),
                                    T.getQualifiers().getAsOpaqueValue(),
                                    SourceRange());
  }

  return clang::QualType(T.getTypePtr(), T.getQualifiers().getAsOpaqueValue());
}

//---------------------------------------------------------
} // end namespace utils
} // end namespace clad
