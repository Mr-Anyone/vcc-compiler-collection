#include "core/codegen.h"

#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>

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
            return emitCallStatement(stmt->getAs<CallStatement>(), holder);
        case code::FunctionArgLists:
            return emitFunctionArgListsStatement(stmt->getAs<FunctionArgLists>(), holder);
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
            VCC_UNREACHABLE("unhandled statement kind in CodeGenerator::emitStatement");
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
            VCC_UNREACHABLE(
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
            VCC_UNREACHABLE(
                "unhandled locator expression in "
                "CodeGenerator::emitRefExpression");
            return nullptr;
    }
}

// ======================================================
// ============= Statement emitters ====================

void CodeGenerator::emitCallStatement(CallStatement* stmt, ContextHolder holder)
{
    emitExpression(stmt->getCallExpression(), holder);
}

void CodeGenerator::emitFunctionArgListsStatement(FunctionArgLists* args,
                                                  ContextHolder holder)
{
    const FunctionDecl* func = args->getFirstFunctionDecl();

    int count                     = 0;
    llvm::Function* llvm_function = getLLVMFunction(func);
    for (llvm::Argument& arg : llvm_function->args())
    {
        const std::string& name = args->getArgs()[count].name;
        arg.setName(name);

        llvm::Value* alloc_loc = holder->builder.CreateAlloca(arg.getType());
        holder->builder.CreateStore(&arg, alloc_loc);

        holder->symbol_table.addLocalVariable(args, name, args->getArgs()[count].type,
                                              alloc_loc);
        ++count;
    }
}

void CodeGenerator::emitAssignmentStatement(AssignmentStatement* stmt,
                                            ContextHolder holder)
{
    VCC_ASSERT(isa<LocatorExpression>(stmt->getRefExpression()) &&
               "must be an locator value");
    llvm::Value* expression_val = emitExpression(stmt->getExpression(), holder);
    llvm::Value* alloc_loc =
        emitRefExpression(dyncast<LocatorExpression>(stmt->getRefExpression()), holder);

    if (!Type::isSame(stmt->getExpression()->getType(holder),
                      stmt->getRefExpression()->getType(holder)))
    {
        holder->diagnostics.diag(stmt, holder->getLine(stmt->getPos()), "invalid type");
        std::exit(-1);
        return;
    }

    VCC_ASSERT(expression_val && alloc_loc);
    holder->builder.CreateStore(expression_val, alloc_loc);
}

void CodeGenerator::emitReturnStatement(ReturnStatement* stmt, ContextHolder holder)
{
    if (stmt->getExpression())
    {
        llvm::Value* return_value = emitExpression(stmt->getExpression(), holder);
        holder->builder.CreateRet(return_value);
    }
    else
    {
        VCC_ASSERT(stmt->getFirstFunctionDecl()->getReturnType()->isVoid() &&
                   "must be void for this to make sense");
        holder->builder.CreateRetVoid();
    }
}

void CodeGenerator::emitDeclarationStatement(DeclarationStatement* stmt,
                                             ContextHolder holder)
{
    llvm::Value* alloc_loc =
        holder->symbol_table.lookupLocalVariable(stmt, stmt->getName()).value;

    if (stmt->getExpression())
    {
        if (!Type::isSame(stmt->getType(), stmt->getExpression()->getType(holder)))
        {
            holder->diagnostics.diag(stmt, holder->getLine(stmt->getPos()),
                                     "type mismatch");
            std::exit(-1);
            return;
        }
        llvm::Value* exp = emitExpression(stmt->getExpression(), holder);
        holder->builder.CreateStore(exp, alloc_loc);
    }
}

void CodeGenerator::emitIfStatement(IfStatement* stmt, ContextHolder holder)
{
    llvm::Function* function = getLLVMFunction(stmt->getFirstFunctionDecl());
    llvm::BasicBlock* true_if_block =
        llvm::BasicBlock::Create(holder->context, "", function);
    llvm::BasicBlock* fallthrough_block =
        llvm::BasicBlock::Create(holder->context, "", function);

    llvm::Value* cond = emitExpression(stmt->getCondition(), holder);
    VCC_ASSERT(cond->getType()->isIntegerTy() && "must be integer type");
    cond = holder->builder.CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0));

    holder->builder.CreateCondBr(cond, true_if_block, fallthrough_block);

    holder->builder.SetInsertPoint(true_if_block);
    for (Statement* statement : stmt->getStatements())
    {
        emitStatement(statement, holder);
    }

    VCC_ASSERT(stmt->getStatements().size() >= 1 && "must be true for now");
    ASTBase* last_expression = stmt->getStatements()[stmt->getStatements().size() - 1];
    if (dynamic_cast<ReturnStatement*>(last_expression) == nullptr)
        holder->builder.CreateBr(fallthrough_block);

    holder->builder.SetInsertPoint(fallthrough_block);
}

void CodeGenerator::emitWhileStatement(WhileStatement* stmt, ContextHolder holder)
{
    llvm::Function* function = getLLVMFunction(stmt->getFirstFunctionDecl());
    llvm::BasicBlock* cond_block =
        llvm::BasicBlock::Create(holder->context, "", function);
    llvm::BasicBlock* while_true_block =
        llvm::BasicBlock::Create(holder->context, "", function);
    llvm::BasicBlock* fallthrough =
        llvm::BasicBlock::Create(holder->context, "", function);

    holder->builder.CreateBr(cond_block);

    holder->builder.SetInsertPoint(cond_block);
    llvm::Value* cond = emitExpression(stmt->getCondition(), holder);
    VCC_ASSERT(cond->getType()->isIntegerTy() && "must be integer");
    cond = holder->builder.CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0));
    holder->builder.CreateCondBr(cond, while_true_block, fallthrough);

    holder->builder.SetInsertPoint(while_true_block);
    for (Statement* statement : stmt->getStatements())
    {
        emitStatement(statement, holder);
    }

    VCC_ASSERT(stmt->getStatements().size() >= 1 && "must be true for now");
    Statement* last_statement = stmt->getStatements()[stmt->getStatements().size() - 1];
    if (!isa<ReturnStatement>(last_statement))
        holder->builder.CreateBr(cond_block);

    holder->builder.SetInsertPoint(fallthrough);
}

void CodeGenerator::emitExternalDeclStatement(FunctionDecl* decl, ContextHolder holder)
{
    llvm::FunctionType* function_type = decl->getFunctionType(holder);
    holder->symbol_table.addFunction(decl);
    m_function_map[decl] = llvm::Function::Create(
        function_type, llvm::Function::ExternalLinkage, decl->getName(), holder->module);
}

void CodeGenerator::emitAllocsStatement(FunctionDecl* decl, ContextHolder holder)
{
    std::vector<DeclarationStatement*> declaration_statements{};

    for (Statement* statement : decl->getStatements())
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
    if (decl->isExtern())
        return emitExternalDeclStatement(decl, holder);

    llvm::FunctionType* function_type = decl->getFunctionType(holder);

    m_function_map[decl] = llvm::Function::Create(
        function_type, llvm::Function::ExternalLinkage, decl->getName(), holder->module);
    llvm::Function* llvm_func = getLLVMFunction(decl);
    llvm_func->setDSOLocal(true);

    holder->symbol_table.addFunction(decl);

    llvm::BasicBlock* block = llvm::BasicBlock::Create(holder->context, "", llvm_func);
    holder->builder.SetInsertPoint(block);

    emitFunctionArgListsStatement(decl->getArgList(), holder);
    emitAllocsStatement(decl, holder);

    for (Statement* statement : decl->getStatements())
    {
        emitStatement(statement, holder);
    }
}

// ======================================================
// ============= Expression emitters ===================

llvm::Value* CodeGenerator::emitConstantExpression(ConstantExpr* expr,
                                                   ContextHolder holder)
{
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context),
                                  expr->getValue());
}

llvm::Value* CodeGenerator::emitCallExpression(CallExpr* expr, ContextHolder holder)
{
    const FunctionDecl* function_decl =
        holder->symbol_table.lookupFunction(expr->getFuncName());

    VCC_ASSERT(function_decl && "this must exist for codegen!");
    VCC_ASSERT(function_decl->getFunctionType(holder)->getNumParams() ==
                   expr->getExpressions().size() &&
               "expected the same number of argument");

    std::vector<llvm::Value*> args;
    int count = 0;
    for (auto it = function_decl->getArgBegin(), ie = function_decl->getArgsEnd();
         it != ie; ++it)
    {
        if (!Type::isSame(it->type, expr->getExpressions()[count]->getType(holder)))
        {
            holder->diagnostics.diag(expr, holder->getLine(expr->getPos()),
                                     "type mismatch");
            std::exit(-1);
        }

        args.push_back(emitExpression(expr->getExpressions()[count], holder));
        ++count;
    }

    if (count != static_cast<int>(expr->getExpressions().size()))
    {
        holder->diagnostics.diag(expr, holder->getLine(expr->getPos()),
                                 "number of argument mismatch");
        std::exit(-1);
    }

    return holder->builder.CreateCall(function_decl->getFunctionType(holder),
                                      getLLVMFunction(function_decl), args);
}

llvm::Value* CodeGenerator::emitIntegerBinaryExpression(BinaryExpression* expr,
                                                        ContextHolder holder,
                                                        llvm::Value* left_hand_side,
                                                        llvm::Value* right_hand_side)
{
    VCC_ASSERT(right_hand_side->getType()->isIntegerTy() &&
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

    VCC_ASSERT(right_hand_side && left_hand_side && "cannot be null");
    VCC_ASSERT(left_hand_side->getType() == right_hand_side->getType());
    switch (expr->getKind())
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
            VCC_ASSERT(left_hand_side->getType()->isIntegerTy() &&
                       right_hand_side->getType()->isIntegerTy());
            return holder->builder.CreateUDiv(left_hand_side, right_hand_side);
        default:
            VCC_UNREACHABLE("cannot get here");
            return nullptr;
    }
}

llvm::Value* CodeGenerator::emitBinaryExpression(BinaryExpression* expr,
                                                 ContextHolder holder)
{
    llvm::Value* right_hand_side = emitExpression(expr->getRHS(), holder);
    llvm::Value* left_hand_side  = emitExpression(expr->getLHS(), holder);

    if (right_hand_side->getType()->isIntegerTy() &&
        left_hand_side->getType()->isIntegerTy())
        return emitIntegerBinaryExpression(expr, holder, left_hand_side, right_hand_side);

    VCC_ASSERT((right_hand_side->getType()->isFloatingPointTy() ||
                left_hand_side->getType()->isFloatingPointTy()) &&
               "either side must be a floating point");

    if (!right_hand_side->getType()->isFloatingPointTy())
    {
        right_hand_side =
            holder->builder.CreateSIToFP(right_hand_side, left_hand_side->getType());
    }
    else if (!left_hand_side->getType()->isFloatingPointTy())
    {
        VCC_ASSERT(left_hand_side->getType()->isIntegerTy());
        left_hand_side =
            holder->builder.CreateSIToFP(left_hand_side, right_hand_side->getType());
    }

    VCC_ASSERT(right_hand_side && left_hand_side && "cannot be null");
    VCC_ASSERT(left_hand_side->getType() == right_hand_side->getType());
    switch (expr->getKind())
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
            VCC_UNREACHABLE("cannot get here");
            return nullptr;
    }
}

llvm::Value* CodeGenerator::emitIdentifierExpression(IdentifierExpr* expr,
                                                     ContextHolder holder)
{
    llvm::Value* loc_value =
        holder->symbol_table.lookupLocalVariable(expr, expr->getName()).value;

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
        (expr->getParentExpression() == nullptr)
            ? holder->symbol_table.lookupLocalVariable(expr, expr->getBaseName()).value
            : emitStartOfPointerFromParentExpression(expr->getParentExpression(), holder);

    Type* current_type = expr->getGEPType(holder);
    int field_num =
        current_type->getAs<StructType>()->getElement(expr->getMember())->field_num;
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
        (expr->getParentExpression() == nullptr)
            ? holder->symbol_table.lookupLocalVariable(expr, expr->getBaseName()).value
            : emitStartOfPointerFromParentExpression(expr->getParentExpression(), holder);

    Type* type            = expr->getGEPType(holder);
    llvm::Type* llvm_type = expr->getGEPType(holder)->getType(holder);
    if (!type->isArray())
        start_of_pointer = holder->builder.CreateLoad(
            llvm::PointerType::get(holder->context, /*AddressSpace*/ 0),
            start_of_pointer);

    llvm::ConstantInt* zero =
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(holder->context), 0);
    llvm::Value* offset = emitExpression(expr->getIndexExpression(), holder);

    return holder->builder.CreateGEP(llvm_type, start_of_pointer,
                                     (type->isArray())
                                         ? std::vector<llvm::Value*>{zero, offset}
                                         : std::vector<llvm::Value*>{offset});
}

llvm::Value* CodeGenerator::emitCurrentRefDeRefExpression(DeRefExpression* expr,
                                                          ContextHolder holder)
{
    VCC_ASSERT(expr->getRef()->getType(holder)->isPointer());
    return emitExpression(expr->getRef(), holder);
}

llvm::Value* CodeGenerator::emitMemberAccessExpression(MemberAccessExpression* expr,
                                                       ContextHolder holder)
{
    if (expr->getChildPosfixExpression())
        return emitExpression(expr->getChildPosfixExpression(), holder);

    llvm::Value* ref_loc   = emitCurrentRefMemberAccessExpression(expr, holder);
    llvm::Type* child_type = expr->getGEPType(holder)
                                 ->getAs<StructType>()
                                 ->getElement(expr->getMember())
                                 ->type->getType(holder);
    return holder->builder.CreateLoad(child_type, ref_loc);
}

llvm::Value* CodeGenerator::emitArrayAccessExpression(ArrayAccessExpression* expr,
                                                      ContextHolder holder)
{
    if (expr->getChildPosfixExpression())
        return emitExpression(expr->getChildPosfixExpression(), holder);

    llvm::Value* start_of_pointer = emitCurrentRefArrayAccessExpression(expr, holder);
    Type* current_type            = expr->getGEPType(holder);
    Type* child_type =
        current_type->isBuiltin() ? current_type : expr->getGEPChildType(holder);
    return holder->builder.CreateLoad(child_type->getType(holder), start_of_pointer);
}

llvm::Value* CodeGenerator::emitDeRefExpression(DeRefExpression* expr,
                                                ContextHolder holder)
{
    if (expr->getPosfixChild())
        return emitExpression(expr->getPosfixChild(), holder);

    VCC_ASSERT(expr->getRef()->getType(holder)->isPointer());
    llvm::Value* current_value = emitExpression(expr->getRef(), holder);
    llvm::Type* base_type =
        expr->getRef()->getType(holder)->getAs<PointerType>()->getPointee()->getType(
            holder);

    return holder->builder.CreateLoad(base_type, current_value);
}

llvm::Value* CodeGenerator::emitRefExpression(RefExpression* expr, ContextHolder holder)
{
    return emitRefExpression(dyncast<LocatorExpression>(expr->getInnerExpression()),
                             holder);
}

llvm::Value* CodeGenerator::emitStringLiteralExpression(StringLiteral* expr,
                                                        ContextHolder holder)
{
    return holder->builder.CreateGlobalString(expr->getString());
}

llvm::Value* CodeGenerator::emitBuiltinCastExpression(CastExpression* expr,
                                                      BuiltinType* from, BuiltinType* to,
                                                      ContextHolder holder)
{
    VCC_ASSERT(from && to);
    VCC_ASSERT(!Type::isSame(from, to));

    if (from->isIntegerKind() && to->isFloat())
    {
        llvm::Value* value = emitExpression(expr->getCastedExpression(), holder);
        return holder->builder.CreateSIToFP(value, to->getType(holder));
    }

    if (from->isFloat() && to->isIntegerKind())
    {
        llvm::Value* value = emitExpression(expr->getCastedExpression(), holder);
        return holder->builder.CreateFPToSI(value, to->getType(holder));
    }

    VCC_ASSERT(from->isIntegerKind() && to->isIntegerKind());
    VCC_ASSERT(from->getBitSize() != to->getBitSize());

    if (from->isBool())
    {
        llvm::Value* val = emitExpression(expr->getCastedExpression(), holder);
        return holder->builder.CreateZExt(val, to->getType(holder));
    }

    if (from->getBitSize() > to->getBitSize())
    {
        llvm::Value* val = emitExpression(expr->getCastedExpression(), holder);
        return holder->builder.CreateTrunc(val, to->getType(holder));
    }

    llvm::Value* val = emitExpression(expr->getCastedExpression(), holder);
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
    Type* from_type = expr->getCastedExpression()->getType(holder);
    if (Type::isSame(from_type, expr->getCastTo()))
        return emitExpression(expr->getCastedExpression(), holder);

    if (from_type->isBuiltin() && expr->getCastTo()->isBuiltin())
        return emitBuiltinCastExpression(expr, from_type->getAs<BuiltinType>(),
                                         expr->getCastTo()->getAs<BuiltinType>(), holder);

    if ((from_type->isPointer() && expr->getCastTo()->isVoidPtr()) ||
        (from_type->isVoidPtr() && expr->getCastTo()->isPointer()))
        return emitExpression(expr->getCastedExpression(), holder);

    emitCastErrorAndExitExpression(expr, holder);
    return nullptr;
}

// ======================================================
// ============= Ref expression emitters ===============

llvm::Value* CodeGenerator::emitRefIdentifierExpression(IdentifierExpr* expr,
                                                        ContextHolder holder)
{
    return holder->symbol_table.lookupLocalVariable(expr, expr->getName()).value;
}

llvm::Value* CodeGenerator::emitRefMemberAccessExpression(MemberAccessExpression* expr,
                                                          ContextHolder holder)
{
    if (expr->getChildPosfixExpression())
        return emitRefExpression(expr->getChildPosfixExpression(), holder);

    return emitCurrentRefMemberAccessExpression(expr, holder);
}

llvm::Value* CodeGenerator::emitRefArrayAccessExpression(ArrayAccessExpression* expr,
                                                         ContextHolder holder)
{
    if (expr->getChildPosfixExpression())
        return emitRefExpression(expr->getChildPosfixExpression(), holder);

    return emitCurrentRefArrayAccessExpression(expr, holder);
}

llvm::Value* CodeGenerator::emitRefDeRefExpression(DeRefExpression* expr,
                                                   ContextHolder holder)
{
    if (expr->getPosfixChild())
        return emitRefExpression(expr->getPosfixChild(), holder);

    VCC_ASSERT(expr->getRef()->getType(holder)->isPointer());
    return emitExpression(expr->getRef(), holder);
}

llvm::Value* CodeGenerator::emitRefRefExpression(RefExpression* expr,
                                                 ContextHolder holder)
{
    VCC_UNREACHABLE("this is ill form");
    return emitRefExpression(dyncast<LocatorExpression>(expr->getInnerExpression()),
                             holder);
}

llvm::Function* CodeGenerator::getLLVMFunction(const FunctionDecl* decl) const
{
    auto it = m_function_map.find(decl);
    VCC_ASSERT(it != m_function_map.end() && "function not registered in codegen");
    return it->second;
}
