#ifndef CORE_CODEGEN_H
#define CORE_CODEGEN_H

#include <llvm/IR/Value.h>

#include "core/context.h"
#include "core/ast.h"

namespace vcc {
class CodeGenerator {
public:
  void emitStatement(Statement *stmt, ContextHolder holder);

private:
  llvm::Value *emitExpression(Expression *expr, ContextHolder holder);
  llvm::Value *emitRefExpression(LocatorExpression *expr,
                                 ContextHolder holder);
  // Statement emitters
  void emitCallStatement(CallStatement *stmt, ContextHolder holder);
  void emitFunctionArgListsStatement(FunctionArgLists *args,
                                     ContextHolder holder);
  void emitFunctionDeclStatement(FunctionDecl *decl, ContextHolder holder);
  void emitAssignmentStatement(AssignmentStatement *stmt,
                               ContextHolder holder);
  void emitReturnStatement(ReturnStatement *stmt, ContextHolder holder);
  void emitDeclarationStatement(DeclarationStatement *stmt,
                                ContextHolder holder);
  void emitIfStatement(IfStatement *stmt, ContextHolder holder);
  void emitWhileStatement(WhileStatement *stmt, ContextHolder holder);
  void emitExternalDeclStatement(FunctionDecl *decl, ContextHolder holder);
  void emitAllocsStatement(FunctionDecl *decl, ContextHolder holder);

  // Expression emitters (getVal equivalents)
  llvm::Value *emitConstantExpression(ConstantExpr *expr,
                                      ContextHolder holder);
  llvm::Value *emitCallExpression(CallExpr *expr, ContextHolder holder);
  llvm::Value *emitBinaryExpression(BinaryExpression *expr,
                                    ContextHolder holder);
  llvm::Value *emitCastExpression(CastExpression *expr, ContextHolder holder);
  llvm::Value *emitIdentifierExpression(IdentifierExpr *expr,
                                        ContextHolder holder);
  llvm::Value *emitMemberAccessExpression(MemberAccessExpression *expr,
                                          ContextHolder holder);
  llvm::Value *emitArrayAccessExpression(ArrayAccessExpression *expr,
                                         ContextHolder holder);
  llvm::Value *emitDeRefExpression(DeRefExpression *expr,
                                   ContextHolder holder);
  llvm::Value *emitRefExpression(RefExpression *expr, ContextHolder holder);
  llvm::Value *emitStringLiteralExpression(StringLiteral *expr,
                                           ContextHolder holder);

  // Ref expression emitters (getRef equivalents)
  llvm::Value *emitRefIdentifierExpression(IdentifierExpr *expr,
                                           ContextHolder holder);
  llvm::Value *emitRefMemberAccessExpression(MemberAccessExpression *expr,
                                             ContextHolder holder);
  llvm::Value *emitRefArrayAccessExpression(ArrayAccessExpression *expr,
                                            ContextHolder holder);
  llvm::Value *emitRefDeRefExpression(DeRefExpression *expr,
                                      ContextHolder holder);
  llvm::Value *emitRefRefExpression(RefExpression *expr, ContextHolder holder);

  // Expression helpers
  llvm::Value *emitIntegerBinaryExpression(BinaryExpression *expr,
                                           ContextHolder holder,
                                           llvm::Value *lhs,
                                           llvm::Value *rhs);
  llvm::Value *emitBuiltinCastExpression(CastExpression *expr,
                                         BuiltinType *from, BuiltinType *to,
                                         ContextHolder holder);
  void emitCastErrorAndExitExpression(CastExpression *expr,
                                      ContextHolder holder);
  llvm::Value *emitStartOfPointerFromParentExpression(Expression *expr,
                                                      ContextHolder holder);
  llvm::Value *emitCurrentRefMemberAccessExpression(
      MemberAccessExpression *expr, ContextHolder holder);
  llvm::Value *emitCurrentRefArrayAccessExpression(
      ArrayAccessExpression *expr, ContextHolder holder);
  llvm::Value *emitCurrentRefDeRefExpression(DeRefExpression *expr,
                                             ContextHolder holder);
};

} // namespace vcc

#endif
