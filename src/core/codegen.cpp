#include "core/codegen.h"

#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>

#include <cassert>
#include <iostream>

#include "core/ast.h"
#include "core/type.h"
#include "core/util.h"

using namespace vcc;

// ======================================================
// ============= Dispatch entry points =================

void CodeGenerator::emitStatement(Statement* stmt, ContextHolder holder)
{
    switch (stmt->getCode())
    {
        case code::CallStatement:
            return emitCallStatement(dyncast<CallStatement>(stmt), holder);
        case code::FunctionArgLists:
            return emitFunctionArgListsStatement(dyncast<FunctionArgLists>(stmt), holder);
        case code::FunctionDecl:
            return emitFunctionDeclStatement(dyncast<FunctionDecl>(stmt), holder);
        case code::AssignmentStatement:
            return emitAssignmentStatement(dyncast<AssignmentStatement>(stmt), holder);
        case code::ReturnStatement:
            return emitReturnStatement(dyncast<ReturnStatement>(stmt), holder);
        case code::DeclarationStatement:
            return emitDeclarationStatement(dyncast<DeclarationStatement>(stmt), holder);
        case code::IfStatement:
            return emitIfStatement(dyncast<IfStatement>(stmt), holder);
        case code::WhileStatement:
            return emitWhileStatement(dyncast<WhileStatement>(stmt), holder);
        default:
            assert(false && "unhandled statement kind in CodeGenerator::emitStatement");
    }
}

llvm::Value* CodeGenerator::emitExpression(Expression* expr, ContextHolder holder)
{
    switch (expr->getCode())
    {
        case code::ConstantExpr:
            return emitConstantExpression(dyncast<ConstantExpr>(expr), holder);
        case code::CallExpr:
            return emitCallExpression(dyncast<CallExpr>(expr), holder);
        case code::BinaryExpression:
            return emitBinaryExpression(dyncast<BinaryExpression>(expr), holder);
        case code::CastExpression:
            return emitCastExpression(dyncast<CastExpression>(expr), holder);
        case code::IdentifierExpr:
            return emitIdentifierExpression(dyncast<IdentifierExpr>(expr), holder);
        case code::MemberAccessExpression:
            return emitMemberAccessExpression(dyncast<MemberAccessExpression>(expr),
                                              holder);
        case code::ArrayAccessExpression:
            return emitArrayAccessExpression(dyncast<ArrayAccessExpression>(expr),
                                             holder);
        case code::DeRefExpression:
            return emitDeRefExpression(dyncast<DeRefExpression>(expr), holder);
        case code::RefExpression:
            return emitRefExpression(dyncast<RefExpression>(expr), holder);
        case code::StringLiteral:
            return emitStringLiteralExpression(dyncast<StringLiteral>(expr), holder);
        default:
            assert(false &&
                   "unhandled expression kind in "
                   "CodeGenerator::emitExpression");
            return nullptr;
    }
}

llvm::Value* CodeGenerator::emitRefExpression(LocatorExpression* expr,
                                              ContextHolder holder)
{
    switch (expr->getCode())
    {
        case code::IdentifierExpr:
            return emitRefIdentifierExpression(dyncast<IdentifierExpr>(expr), holder);
        case code::MemberAccessExpression:
            return emitRefMemberAccessExpression(dyncast<MemberAccessExpression>(expr),
                                                 holder);
        case code::ArrayAccessExpression:
            return emitRefArrayAccessExpression(dyncast<ArrayAccessExpression>(expr),
                                                holder);
        case code::DeRefExpression:
            return emitRefDeRefExpression(dyncast<DeRefExpression>(expr), holder);
        case code::RefExpression:
            return emitRefRefExpression(dyncast<RefExpression>(expr), holder);
        default:
            assert(false &&
                   "unhandled locator expression in "
                   "CodeGenerator::emitRefExpression");
            return nullptr;
    }
}

// ======================================================
// ============= Statement emitters ====================

void CodeGenerator::emitCallStatement(CallStatement* stmt, ContextHolder holder)
{
    emitExpression(stmt->m_call_expr, holder);
}

void CodeGenerator::emitFunctionArgListsStatement(FunctionArgLists* args,
                                                  ContextHolder holder)
{
    const FunctionDecl* func = args->getFirstFunctionDecl();

    int count                     = 0;
    llvm::Function* llvm_function = func->getLLVMFunction();
    for (llvm::Argument& arg : llvm_function->args())
    {
        const std::string& name = args->m_args[count].name;
        arg.setName(name);

        llvm::Value* alloc_loc = holder->builder.CreateAlloca(arg.getType());
        holder->builder.CreateStore(&arg, alloc_loc);

        holder->symbol_table.addLocalVariable(args, name, args->m_args[count].type,
                                              alloc_loc);
        ++count;
    }
}

void CodeGenerator::emitAssignmentStatement(AssignmentStatement* stmt,
                                            ContextHolder holder)
{
    assert(isa<LocatorExpression>(stmt->m_ref_expr) && "must be an locator value");
    llvm::Value* expression_val = emitExpression(stmt->m_expression, holder);
    llvm::Value* alloc_loc =
        emitRefExpression(dyncast<LocatorExpression>(stmt->m_ref_expr), holder);

    if (!Type::isSame(stmt->m_expression->getType(holder),
                      stmt->m_ref_expr->getType(holder)))
    {
        holder->diagnostics.diag(stmt, holder->getLine(stmt->getPos()), "invalid type");
        std::exit(-1);
        return;
    }

    assert(expression_val && alloc_loc);
    holder->builder.CreateStore(expression_val, alloc_loc);
}

void CodeGenerator::emitReturnStatement(ReturnStatement* stmt, ContextHolder holder)
{
    if (stmt->m_expression)
    {
        llvm::Value* return_value = emitExpression(stmt->m_expression, holder);
        holder->builder.CreateRet(return_value);
    }
    else
    {
        assert(stmt->getFirstFunctionDecl()->getReturnType()->isVoid() &&
               "must be void for this to make sense");
        holder->builder.CreateRetVoid();
    }
}

void CodeGenerator::emitDeclarationStatement(DeclarationStatement* stmt,
                                             ContextHolder holder)
{
    llvm::Value* alloc_loc =
        holder->symbol_table.lookupLocalVariable(stmt, stmt->m_name).value;

    if (stmt->m_expression)
    {
        if (!Type::isSame(stmt->m_type, stmt->m_expression->getType(holder)))
        {
            holder->diagnostics.diag(stmt, holder->getLine(stmt->getPos()),
                                     "type mismatch");
            std::exit(-1);
            return;
        }
        llvm::Value* exp = emitExpression(stmt->m_expression, holder);
        holder->builder.CreateStore(exp, alloc_loc);
    }
}

void CodeGenerator::emitIfStatement(IfStatement* stmt, ContextHolder holder)
{
    llvm::Function* function = stmt->getFirstFunctionDecl()->getLLVMFunction();
    llvm::BasicBlock* true_if_block =
        llvm::BasicBlock::Create(holder->context, "", function);
    llvm::BasicBlock* fallthrough_block =
        llvm::BasicBlock::Create(holder->context, "", function);

    llvm::Value* cond = emitExpression(stmt->m_cond, holder);
    assert(cond->getType()->isIntegerTy() && "must be integer type");
    cond = holder->builder.CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0));

    holder->builder.CreateCondBr(cond, true_if_block, fallthrough_block);

    holder->builder.SetInsertPoint(true_if_block);
    for (Statement* statement : stmt->m_statements)
    {
        emitStatement(statement, holder);
    }

    assert(stmt->m_statements.size() >= 1 && "must be true for now");
    ASTBase* last_expression = stmt->m_statements[stmt->m_statements.size() - 1];
    if (dynamic_cast<ReturnStatement*>(last_expression) == nullptr)
        holder->builder.CreateBr(fallthrough_block);

    holder->builder.SetInsertPoint(fallthrough_block);
}

void CodeGenerator::emitWhileStatement(WhileStatement* stmt, ContextHolder holder)
{
    llvm::BasicBlock* cond_block = llvm::BasicBlock::Create(
        holder->context, "", stmt->getFirstFunctionDecl()->getLLVMFunction());
    llvm::BasicBlock* while_true_block = llvm::BasicBlock::Create(
        holder->context, "", stmt->getFirstFunctionDecl()->getLLVMFunction());
    llvm::BasicBlock* fallthrough = llvm::BasicBlock::Create(
        holder->context, "", stmt->getFirstFunctionDecl()->getLLVMFunction());

    holder->builder.CreateBr(cond_block);

    holder->builder.SetInsertPoint(cond_block);
    llvm::Value* cond = emitExpression(stmt->m_cond, holder);
    assert(cond->getType()->isIntegerTy() && "must be integer");
    cond = holder->builder.CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0));
    holder->builder.CreateCondBr(cond, while_true_block, fallthrough);

    holder->builder.SetInsertPoint(while_true_block);
    for (Statement* statement : stmt->m_statements)
    {
        emitStatement(statement, holder);
    }

    assert(stmt->m_statements.size() >= 1 && "must be true for now");
    Statement* last_statement = stmt->m_statements[stmt->m_statements.size() - 1];
    if (!isa<ReturnStatement>(last_statement))
        holder->builder.CreateBr(cond_block);

    holder->builder.SetInsertPoint(fallthrough);
}

void CodeGenerator::emitExternalDeclStatement(FunctionDecl* decl, ContextHolder holder)
{
    llvm::FunctionType* function_type = decl->getFunctionType(holder);
    holder->symbol_table.addFunction(decl);
    decl->m_function = llvm::Function::Create(
        function_type, llvm::Function::ExternalLinkage, decl->m_name, holder->module);
}

void CodeGenerator::emitAllocsStatement(FunctionDecl* decl, ContextHolder holder)
{
    std::vector<DeclarationStatement*> declaration_statements{};

    for (Statement* statement : decl->m_statements)
    {
        switch (statement->getCode())
        {
            case code::DeclarationStatement:
            {
                declaration_statements.push_back(
                    dyncast<DeclarationStatement>(statement));
                break;
            }
            case code::WhileStatement:
            {
                std::vector<DeclarationStatement*> added =
                    dyncast<WhileStatement>(statement)->getDeclarationStatements();
                declaration_statements.insert(declaration_statements.end(), added.begin(),
                                              added.end());
                break;
            }
            case code::IfStatement:
            {
                std::vector<DeclarationStatement*> added =
                    dyncast<IfStatement>(statement)->getDeclarationStatements();
                declaration_statements.insert(declaration_statements.end(), added.begin(),
                                              added.end());
            }
            default:
                break;
        }
    }

    for (DeclarationStatement* statement : declaration_statements)
    {
        llvm::Type* llvm_type = statement->getType()->getType(holder);
        llvm::Value* loc      = holder->builder.CreateAlloca(llvm_type);
        holder->symbol_table.addLocalVariable(statement, statement->getName(),
                                              statement->getType(), loc);
    }
}

void CodeGenerator::emitFunctionDeclStatement(FunctionDecl* decl, ContextHolder holder)
{
    if (decl->m_is_extern)
        return emitExternalDeclStatement(decl, holder);

    llvm::FunctionType* function_type = decl->getFunctionType(holder);

    decl->m_function = llvm::Function::Create(
        function_type, llvm::Function::ExternalLinkage, decl->m_name, holder->module);
    decl->m_function->setDSOLocal(true);

    holder->symbol_table.addFunction(decl);

    llvm::BasicBlock* block =
        llvm::BasicBlock::Create(holder->context, "", decl->m_function);
    holder->builder.SetInsertPoint(block);

    emitFunctionArgListsStatement(decl->m_arg_list, holder);
    emitAllocsStatement(decl, holder);

    for (Statement* statement : decl->m_statements)
    {
        emitStatement(statement, holder);
    }
}

// ======================================================
// ============= Expression emitters ===================

llvm::Value* CodeGenerator::emitConstantExpression(ConstantExpr* expr,
                                                   ContextHolder holder)
{
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), expr->m_value);
}

llvm::Value* CodeGenerator::emitCallExpression(CallExpr* expr, ContextHolder holder)
{
    const FunctionDecl* function_decl =
        holder->symbol_table.lookupFunction(expr->m_func_name);

    assert(function_decl && "this must exist for codegen!");
    assert(function_decl->getFunctionType(holder)->getNumParams() ==
               expr->m_expressions.size() &&
           "expected the same number of argument");

    std::vector<llvm::Value*> args;
    int count = 0;
    for (auto it = function_decl->getArgBegin(), ie = function_decl->getArgsEnd();
         it != ie; ++it)
    {
        if (!Type::isSame(it->type, expr->m_expressions[count]->getType(holder)))
        {
            holder->diagnostics.diag(expr, holder->getLine(expr->getPos()),
                                     "type mismatch");
            std::exit(-1);
        }

        args.push_back(emitExpression(expr->m_expressions[count], holder));
        ++count;
    }

    if (count != static_cast<int>(expr->m_expressions.size()))
    {
        holder->diagnostics.diag(expr, holder->getLine(expr->getPos()),
                                 "number of argument mismatch");
        std::exit(-1);
    }

    return holder->builder.CreateCall(function_decl->getFunctionType(holder),
                                      function_decl->getLLVMFunction(), args);
}

llvm::Value* CodeGenerator::emitIntegerBinaryExpression(BinaryExpression* expr,
                                                        ContextHolder holder,
                                                        llvm::Value* left_hand_side,
                                                        llvm::Value* right_hand_side)
{
    assert(right_hand_side->getType()->isIntegerTy() &&
           left_hand_side->getType()->isIntegerTy() &&
           "both side most be integer for now");

    if (right_hand_side->getType()->getPrimitiveSizeInBits() >
        left_hand_side->getType()->getPrimitiveSizeInBits())
    {
        left_hand_side =
            holder->builder.CreateSExt(left_hand_side, right_hand_side->getType());
    }
    else if (left_hand_side->getType()->getPrimitiveSizeInBits() >
             right_hand_side->getType()->getPrimitiveSizeInBits())
    {
        right_hand_side =
            holder->builder.CreateSExt(right_hand_side, left_hand_side->getType());
    }

    assert(right_hand_side && left_hand_side && "cannot be null");
    assert(left_hand_side->getType() == right_hand_side->getType());
    switch (expr->m_kind)
    {
        case BinaryExpression::Add:
            return holder->builder.CreateAdd(left_hand_side, right_hand_side);
        case BinaryExpression::Multiply:
            return holder->builder.CreateMul(left_hand_side, right_hand_side);
        case BinaryExpression::Equal:
            return holder->builder.CreateICmpEQ(left_hand_side, right_hand_side);
        case BinaryExpression::NEquals:
            return holder->builder.CreateICmpNE(left_hand_side, right_hand_side);
        case BinaryExpression::GE:
            return holder->builder.CreateICmpSGE(left_hand_side, right_hand_side);
        case BinaryExpression::GT:
            return holder->builder.CreateICmpSGT(left_hand_side, right_hand_side);
        case BinaryExpression::Subtract:
            return holder->builder.CreateSub(left_hand_side, right_hand_side);
        case BinaryExpression::LE:
            return holder->builder.CreateICmpSLE(left_hand_side, right_hand_side);
        case BinaryExpression::LT:
            return holder->builder.CreateICmpSLT(left_hand_side, right_hand_side);
        case BinaryExpression::Divide:
            assert(left_hand_side->getType()->isIntegerTy() &&
                   right_hand_side->getType()->isIntegerTy());
            return holder->builder.CreateUDiv(left_hand_side, right_hand_side);
        default:
            assert(false && "cannot get here");
            return nullptr;
    }
}

llvm::Value* CodeGenerator::emitBinaryExpression(BinaryExpression* expr,
                                                 ContextHolder holder)
{
    llvm::Value* right_hand_side = emitExpression(expr->m_rhs, holder);
    llvm::Value* left_hand_side  = emitExpression(expr->m_lhs, holder);

    if (right_hand_side->getType()->isIntegerTy() &&
        left_hand_side->getType()->isIntegerTy())
        return emitIntegerBinaryExpression(expr, holder, left_hand_side, right_hand_side);

    assert((right_hand_side->getType()->isFloatingPointTy() ||
            left_hand_side->getType()->isFloatingPointTy()) &&
           "either side must be a floating point");

    if (!right_hand_side->getType()->isFloatingPointTy())
    {
        right_hand_side =
            holder->builder.CreateSIToFP(right_hand_side, left_hand_side->getType());
    }
    else if (!left_hand_side->getType()->isFloatingPointTy())
    {
        assert(left_hand_side->getType()->isIntegerTy());
        left_hand_side =
            holder->builder.CreateSIToFP(left_hand_side, right_hand_side->getType());
    }

    assert(right_hand_side && left_hand_side && "cannot be null");
    assert(left_hand_side->getType() == right_hand_side->getType());
    switch (expr->m_kind)
    {
        case BinaryExpression::Add:
            return holder->builder.CreateFAdd(left_hand_side, right_hand_side);
        case BinaryExpression::Multiply:
            return holder->builder.CreateFMul(left_hand_side, right_hand_side);
        case BinaryExpression::Equal:
            return holder->builder.CreateFCmpOEQ(left_hand_side, right_hand_side);
        case BinaryExpression::NEquals:
            return holder->builder.CreateFCmpONE(left_hand_side, right_hand_side);
        case BinaryExpression::GE:
            return holder->builder.CreateFCmpOGE(left_hand_side, right_hand_side);
        case BinaryExpression::GT:
            return holder->builder.CreateFCmpOGT(left_hand_side, right_hand_side);
        case BinaryExpression::Subtract:
            return holder->builder.CreateFSub(left_hand_side, right_hand_side);
        case BinaryExpression::LE:
            return holder->builder.CreateFCmpOLE(left_hand_side, right_hand_side);
        case BinaryExpression::LT:
            return holder->builder.CreateFCmpOLT(left_hand_side, right_hand_side);
        case BinaryExpression::Divide:
            return holder->builder.CreateFDiv(left_hand_side, right_hand_side);
        default:
            assert(false && "cannot get here");
            return nullptr;
    }
}

llvm::Value* CodeGenerator::emitIdentifierExpression(IdentifierExpr* expr,
                                                     ContextHolder holder)
{
    llvm::Value* loc_value =
        holder->symbol_table.lookupLocalVariable(expr, expr->m_name).value;

    return holder->builder.CreateLoad(expr->getType(holder)->getType(holder), loc_value);
}

llvm::Value* CodeGenerator::emitStartOfPointerFromParentExpression(Expression* expression,
                                                                   ContextHolder holder)
{
    if (MemberAccessExpression* member = dyncast<MemberAccessExpression>(expression))
        return emitCurrentRefMemberAccessExpression(member, holder);

    if (DeRefExpression* ref = dyncast<DeRefExpression>(expression))
        return emitCurrentRefDeRefExpression(ref, holder);

    return emitCurrentRefArrayAccessExpression(dyncast<ArrayAccessExpression>(expression),
                                               holder);
}

llvm::Value* CodeGenerator::emitCurrentRefMemberAccessExpression(
    MemberAccessExpression* expr, ContextHolder holder)
{
    llvm::Value* start_of_pointer =
        (expr->m_parent == nullptr)
            ? holder->symbol_table.lookupLocalVariable(expr, expr->m_base_name).value
            : emitStartOfPointerFromParentExpression(expr->m_parent, holder);

    Type* current_type = expr->getGEPType(holder);
    int field_num =
        current_type->getAs<StructType>()->getElement(expr->m_member)->field_num;
    llvm::Value* zero =
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), 0);
    llvm::Value* offset =
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), field_num);

    return holder->builder.CreateGEP(current_type->getType(holder), start_of_pointer,
                                     {zero, offset});
}

llvm::Value* CodeGenerator::emitCurrentRefArrayAccessExpression(
    ArrayAccessExpression* expr, ContextHolder holder)
{
    llvm::Value* start_of_pointer =
        (expr->m_parent_expression == nullptr)
            ? holder->symbol_table.lookupLocalVariable(expr, expr->m_base_name).value
            : emitStartOfPointerFromParentExpression(expr->m_parent_expression, holder);

    Type* type            = expr->getGEPType(holder);
    llvm::Type* llvm_type = expr->getGEPType(holder)->getType(holder);
    if (!type->isArray())
        start_of_pointer = holder->builder.CreateLoad(
            llvm::PointerType::get(holder->context, /*AddressSpace*/ 0),
            start_of_pointer);

    llvm::ConstantInt* zero =
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), 0);
    llvm::Value* offset = emitExpression(expr->m_index_expression, holder);

    return holder->builder.CreateGEP(llvm_type, start_of_pointer,
                                     (type->isArray())
                                         ? std::vector<llvm::Value*>{zero, offset}
                                         : std::vector<llvm::Value*>{offset});
}

llvm::Value* CodeGenerator::emitCurrentRefDeRefExpression(DeRefExpression* expr,
                                                          ContextHolder holder)
{
    assert(expr->m_ref->getType(holder)->isPointer());
    return emitExpression(expr->m_ref, holder);
}

llvm::Value* CodeGenerator::emitMemberAccessExpression(MemberAccessExpression* expr,
                                                       ContextHolder holder)
{
    if (expr->m_child_posfix_expression)
        return emitExpression(expr->m_child_posfix_expression, holder);

    llvm::Value* ref_loc   = emitCurrentRefMemberAccessExpression(expr, holder);
    llvm::Type* child_type = expr->getGEPType(holder)
                                 ->getAs<StructType>()
                                 ->getElement(expr->m_member)
                                 ->type->getType(holder);
    return holder->builder.CreateLoad(child_type, ref_loc);
}

llvm::Value* CodeGenerator::emitArrayAccessExpression(ArrayAccessExpression* expr,
                                                      ContextHolder holder)
{
    if (expr->m_child_posfix_expression)
        return emitExpression(expr->m_child_posfix_expression, holder);

    llvm::Value* start_of_pointer = emitCurrentRefArrayAccessExpression(expr, holder);
    Type* current_type            = expr->getGEPType(holder);
    Type* child_type =
        current_type->isBuiltin() ? current_type : expr->getGEPChildType(holder);
    return holder->builder.CreateLoad(child_type->getType(holder), start_of_pointer);
}

llvm::Value* CodeGenerator::emitDeRefExpression(DeRefExpression* expr,
                                                ContextHolder holder)
{
    if (expr->m_posfix_child)
        return emitExpression(expr->m_posfix_child, holder);

    assert(expr->m_ref->getType(holder)->isPointer());
    llvm::Value* current_value = emitExpression(expr->m_ref, holder);
    llvm::Type* base_type =
        expr->m_ref->getType(holder)->getAs<PointerType>()->getPointee()->getType(holder);

    return holder->builder.CreateLoad(base_type, current_value);
}

llvm::Value* CodeGenerator::emitRefExpression(RefExpression* expr, ContextHolder holder)
{
    return emitRefExpression(dyncast<LocatorExpression>(expr->m_inner_expression),
                             holder);
}

llvm::Value* CodeGenerator::emitStringLiteralExpression(StringLiteral* expr,
                                                        ContextHolder holder)
{
    return holder->builder.CreateGlobalString(expr->m_string_literal);
}

llvm::Value* CodeGenerator::emitBuiltinCastExpression(CastExpression* expr,
                                                      BuiltinType* from, BuiltinType* to,
                                                      ContextHolder holder)
{
    assert(from && to);
    assert(!Type::isSame(from, to));

    if (from->isIntegerKind() && to->isFloat())
    {
        llvm::Value* value = emitExpression(expr->m_to_be_casted_expression, holder);
        return holder->builder.CreateSIToFP(value, to->getType(holder));
    }

    if (from->isFloat() && to->isIntegerKind())
    {
        llvm::Value* value = emitExpression(expr->m_to_be_casted_expression, holder);
        return holder->builder.CreateFPToSI(value, to->getType(holder));
    }

    assert(from->isIntegerKind() && to->isIntegerKind());
    assert(from->getBitSize() != to->getBitSize());

    if (from->isBool())
    {
        llvm::Value* val = emitExpression(expr->m_to_be_casted_expression, holder);
        return holder->builder.CreateZExt(val, to->getType(holder));
    }

    if (from->getBitSize() > to->getBitSize())
    {
        llvm::Value* val = emitExpression(expr->m_to_be_casted_expression, holder);
        return holder->builder.CreateTrunc(val, to->getType(holder));
    }

    llvm::Value* val = emitExpression(expr->m_to_be_casted_expression, holder);
    return holder->builder.CreateSExt(val, to->getType(holder));
}

void CodeGenerator::emitCastErrorAndExitExpression(CastExpression* expr,
                                                   ContextHolder holder)
{
    holder->diagnostics.diag(expr, holder->getLine(expr->getPos()),
                             "cannot perform a cast");
    std::exit(-1);
}

llvm::Value* CodeGenerator::emitCastExpression(CastExpression* expr, ContextHolder holder)
{
    Type* from_type = expr->m_to_be_casted_expression->getType(holder);
    if (Type::isSame(from_type, expr->m_cast_to))
        return emitExpression(expr->m_to_be_casted_expression, holder);

    if (from_type->isBuiltin() && expr->m_cast_to->isBuiltin())
        return emitBuiltinCastExpression(expr, from_type->getAs<BuiltinType>(),
                                         expr->m_cast_to->getAs<BuiltinType>(), holder);

    if ((from_type->isPointer() && expr->m_cast_to->isVoidPtr()) ||
        (from_type->isVoidPtr() && expr->m_cast_to->isPointer()))
        return emitExpression(expr->m_to_be_casted_expression, holder);

    emitCastErrorAndExitExpression(expr, holder);
    return nullptr;
}

// ======================================================
// ============= Ref expression emitters ===============

llvm::Value* CodeGenerator::emitRefIdentifierExpression(IdentifierExpr* expr,
                                                        ContextHolder holder)
{
    return holder->symbol_table.lookupLocalVariable(expr, expr->m_name).value;
}

llvm::Value* CodeGenerator::emitRefMemberAccessExpression(MemberAccessExpression* expr,
                                                          ContextHolder holder)
{
    if (expr->m_child_posfix_expression)
        return emitRefExpression(expr->m_child_posfix_expression, holder);

    return emitCurrentRefMemberAccessExpression(expr, holder);
}

llvm::Value* CodeGenerator::emitRefArrayAccessExpression(ArrayAccessExpression* expr,
                                                         ContextHolder holder)
{
    if (expr->m_child_posfix_expression)
        return emitRefExpression(expr->m_child_posfix_expression, holder);

    return emitCurrentRefArrayAccessExpression(expr, holder);
}

llvm::Value* CodeGenerator::emitRefDeRefExpression(DeRefExpression* expr,
                                                   ContextHolder holder)
{
    if (expr->m_posfix_child)
        return emitRefExpression(expr->m_posfix_child, holder);

    assert(expr->m_ref->getType(holder)->isPointer());
    return emitExpression(expr->m_ref, holder);
}

llvm::Value* CodeGenerator::emitRefRefExpression(RefExpression* expr,
                                                 ContextHolder holder)
{
    assert(false && "this is ill form");
    return emitRefExpression(dyncast<LocatorExpression>(expr->m_inner_expression),
                             holder);
}
