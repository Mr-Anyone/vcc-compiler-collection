#ifndef CORE_CODEGEN_H
#define CORE_CODEGEN_H

#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Value.h>

#include "core/ast.h"
#include "core/context.h"
#include "core/symbol_table.h"

namespace vcc
{
using CGSymbolTable = SymbolTable<llvm::Value*>;

class CodeGenerator
{
   public:
    CodeGenerator(ContextHolder holder);
    void emitStatement(Statement* stmt);

   private:
    llvm::Value* emitExpression(Expression* expr);
    llvm::Value* emitRefExpression(LocatorExpression* expr);
    // Statement emitters
    void emitCallStatement(CallStatement* stmt);
    void emitFunctionArgListsStatement(FunctionArgLists* args);
    void emitFunctionDeclStatement(FunctionDecl* decl);
    void emitAssignmentStatement(AssignmentStatement* stmt);
    void emitReturnStatement(ReturnStatement* stmt);
    void emitDeclarationStatement(DeclarationStatement* stmt);
    void emitIfStatement(IfStatement* stmt);
    void emitWhileStatement(WhileStatement* stmt);
    void emitExternalDeclStatement(FunctionDecl* decl);
    void emitAllocsStatement(FunctionDecl* decl);

    llvm::Value* emitConstantExpression(ConstantExpr* expr);
    llvm::Value* emitCallExpression(CallExpr* expr);
    llvm::Value* emitBinaryExpression(BinaryExpression* expr);
    llvm::Value* emitCastExpression(CastExpression* expr);
    llvm::Value* emitIdentifierExpression(IdentifierExpr* expr);
    llvm::Value* emitMemberAccessExpression(MemberAccessExpression* expr);
    llvm::Value* emitArrayAccessExpression(ArrayAccessExpression* expr);
    llvm::Value* emitDeRefExpression(DeRefExpression* expr);
    llvm::Value* emitRefExpression(RefExpression* expr);
    llvm::Value* emitStringLiteralExpression(StringLiteral* expr);

    llvm::Value* emitRefIdentifierExpression(IdentifierExpr* expr);
    llvm::Value* emitRefMemberAccessExpression(MemberAccessExpression* expr);
    llvm::Value* emitRefArrayAccessExpression(ArrayAccessExpression* expr);
    llvm::Value* emitRefDeRefExpression(DeRefExpression* expr);
    llvm::Value* emitRefRefExpression(RefExpression* expr);

    llvm::Value* emitIntegerBinaryExpression(BinaryExpression* expr, llvm::Value* lhs,
                                             llvm::Value* rhs);
    llvm::Value* emitBuiltinCastExpression(CastExpression* expr, BuiltinType* from,
                                           BuiltinType* to);
    llvm::Value* emitStartOfPointerFromParentExpression(Expression* expr);
    llvm::Value* emitCurrentRefMemberAccessExpression(MemberAccessExpression* expr);
    llvm::Value* emitCurrentRefArrayAccessExpression(ArrayAccessExpression* expr);
    llvm::Value* emitCurrentRefDeRefExpression(DeRefExpression* expr);

    llvm::Function* getLLVMFunction(const FunctionDecl* decl) const;

    llvm::LLVMContext& llvmContext();
    ContextHolder context();
    llvm::IRBuilder<>& builder();
    CGSymbolTable& symbolTable();

    llvm::DenseMap<const FunctionDecl*, llvm::Function*> m_function_map;
    ContextHolder m_context;
    CGSymbolTable m_symbol_table;
};

}  // namespace vcc

#endif
