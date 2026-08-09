#include "core/codegen.h"

#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>

#include "core/ast.h"
#include "core/type.h"
#include "core/util.h"

using namespace vcc;

CodeGenerator::CodeGenerator(ContextHolder holder) : m_context(holder) {}

// ======================================================
// ============= Dispatch entry points =================

void CodeGenerator::emitStatement(Statement* stmt)
{
    switch (stmt->getCode())
    {
        case code::CallStatement:
            return emitCallStatement(stmt->getAs<CallStatement>());
        case code::FunctionArgLists:
            return emitFunctionArgListsStatement(stmt->getAs<FunctionArgLists>());
        case code::FunctionDecl:
            return emitFunctionDeclStatement(dyncast<FunctionDecl>(stmt));
        case code::AssignmentStatement:
            return emitAssignmentStatement(dyncast<AssignmentStatement>(stmt));
        case code::ReturnStatement:
            return emitReturnStatement(dyncast<ReturnStatement>(stmt));
        case code::DeclarationStatement:
            return emitDeclarationStatement(dyncast<DeclarationStatement>(stmt));
        case code::IfStatement:
            return emitIfStatement(dyncast<IfStatement>(stmt));
        case code::WhileStatement:
            return emitWhileStatement(dyncast<WhileStatement>(stmt));
        default:
            VCC_UNREACHABLE("unhandled statement kind in CodeGenerator::emitStatement");
    }
}

llvm::Value* CodeGenerator::emitExpression(Expression* expr)
{
    switch (expr->getCode())
    {
        case code::ConstantExpr:
            return emitConstantExpression(dyncast<ConstantExpr>(expr));
        case code::CallExpr:
            return emitCallExpression(dyncast<CallExpr>(expr));
        case code::BinaryExpression:
            return emitBinaryExpression(dyncast<BinaryExpression>(expr));
        case code::CastExpression:
            return emitCastExpression(dyncast<CastExpression>(expr));
        case code::IdentifierExpr:
            return emitIdentifierExpression(dyncast<IdentifierExpr>(expr));
        case code::MemberAccessExpression:
            return emitMemberAccessExpression(dyncast<MemberAccessExpression>(expr));
        case code::ArrayAccessExpression:
            return emitArrayAccessExpression(dyncast<ArrayAccessExpression>(expr));
        case code::DeRefExpression:
            return emitDeRefExpression(dyncast<DeRefExpression>(expr));
        case code::RefExpression:
            return emitRefExpression(dyncast<RefExpression>(expr));
        case code::StringLiteral:
            return emitStringLiteralExpression(dyncast<StringLiteral>(expr));
        default:
            VCC_UNREACHABLE(
                "unhandled expression kind in "
                "CodeGenerator::emitExpression");
            return nullptr;
    }
}

llvm::Value* CodeGenerator::emitRefExpression(LocatorExpression* expr)
{
    switch (expr->getCode())
    {
        case code::IdentifierExpr:
            return emitRefIdentifierExpression(dyncast<IdentifierExpr>(expr));
        case code::MemberAccessExpression:
            return emitRefMemberAccessExpression(dyncast<MemberAccessExpression>(expr));
        case code::ArrayAccessExpression:
            return emitRefArrayAccessExpression(dyncast<ArrayAccessExpression>(expr));
        case code::DeRefExpression:
            return emitRefDeRefExpression(dyncast<DeRefExpression>(expr));
        case code::RefExpression:
            return emitRefRefExpression(dyncast<RefExpression>(expr));
        default:
            VCC_UNREACHABLE(
                "unhandled locator expression in "
                "CodeGenerator::emitRefExpression");
            return nullptr;
    }
}

// ======================================================
// ============= Statement emitters ====================

void CodeGenerator::emitCallStatement(CallStatement* stmt)
{
    emitExpression(stmt->getCallExpression());
}

void CodeGenerator::emitFunctionArgListsStatement(FunctionArgLists* args)
{
    const FunctionDecl* func = args->getFirstFunctionDecl();

    int count                     = 0;
    llvm::Function* llvm_function = getLLVMFunction(func);
    for (llvm::Argument& arg : llvm_function->args())
    {
        const std::string& name = args->getArgs()[count].name;
        arg.setName(name);

        llvm::Value* alloc_loc = builder().CreateAlloca(arg.getType());
        builder().CreateStore(&arg, alloc_loc);

        symbolTable().insert(args, name, alloc_loc);
        ++count;
    }
}

void CodeGenerator::emitAssignmentStatement(AssignmentStatement* stmt)
{
    VCC_ASSERT(isa<LocatorExpression>(stmt->getRefExpression()) &&
               "must be an locator value");
    llvm::Value* expression_val = emitExpression(stmt->getExpression());
    llvm::Value* alloc_loc =
        emitRefExpression(dyncast<LocatorExpression>(stmt->getRefExpression()));

    if (!Type::isSame(stmt->getExpression()->getType(context()),
                      stmt->getRefExpression()->getType(context())))
    {
        VCC_UNREACHABLE("please implement m_context->getLine");
        // m_context->diagnostics.diag(stmt, m_context->getLine(stmt->getPos()), "invalid
        // type");
        std::exit(-1);
        return;
    }

    VCC_ASSERT(expression_val && alloc_loc);
    builder().CreateStore(expression_val, alloc_loc);
}

void CodeGenerator::emitReturnStatement(ReturnStatement* stmt)
{
    if (stmt->getExpression())
    {
        llvm::Value* return_value = emitExpression(stmt->getExpression());
        builder().CreateRet(return_value);
    }
    else
    {
        VCC_ASSERT(stmt->getFirstFunctionDecl()->getReturnType()->isVoid() &&
                   "must be void for this to make sense");
        builder().CreateRetVoid();
    }
}

void CodeGenerator::emitDeclarationStatement(DeclarationStatement* stmt)
{
    llvm::Value* alloc_loc = symbolTable().lookup(stmt, stmt->getName()).value();

    if (stmt->getExpression())
    {
        if (!Type::isSame(stmt->getType(), stmt->getExpression()->getType(context())))
        {
            VCC_UNREACHABLE("imlement m_context->getLine");
            // m_context->diagnostics.diag(stmt, m_context->getLine(stmt->getPos()),
            //                          "type mismatch");
            // std::exit(-1);
            return;
        }
        llvm::Value* exp = emitExpression(stmt->getExpression());
        builder().CreateStore(exp, alloc_loc);
    }
}

void CodeGenerator::emitIfStatement(IfStatement* stmt)
{
    llvm::Function* function = getLLVMFunction(stmt->getFirstFunctionDecl());
    llvm::BasicBlock* true_if_block =
        llvm::BasicBlock::Create(llvmContext(), "", function);
    llvm::BasicBlock* fallthrough_block =
        llvm::BasicBlock::Create(llvmContext(), "", function);

    llvm::Value* cond = emitExpression(stmt->getCondition());
    VCC_ASSERT(cond->getType()->isIntegerTy() && "must be integer type");
    cond = builder().CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0));

    builder().CreateCondBr(cond, true_if_block, fallthrough_block);

    builder().SetInsertPoint(true_if_block);
    for (Statement* statement : stmt->getStatements())
    {
        emitStatement(statement);
    }

    VCC_ASSERT(stmt->getStatements().size() >= 1 && "must be true for now");
    ASTBase* last_expression = stmt->getStatements()[stmt->getStatements().size() - 1];
    if (dynamic_cast<ReturnStatement*>(last_expression) == nullptr)
        builder().CreateBr(fallthrough_block);

    builder().SetInsertPoint(fallthrough_block);
}

void CodeGenerator::emitWhileStatement(WhileStatement* stmt)
{
    llvm::Function* function = getLLVMFunction(stmt->getFirstFunctionDecl());
    llvm::BasicBlock* cond_block = llvm::BasicBlock::Create(llvmContext(), "", function);
    llvm::BasicBlock* while_true_block =
        llvm::BasicBlock::Create(llvmContext(), "", function);
    llvm::BasicBlock* fallthrough = llvm::BasicBlock::Create(llvmContext(), "", function);

    builder().CreateBr(cond_block);
    builder().SetInsertPoint(cond_block);
    llvm::Value* cond = emitExpression(stmt->getCondition());
    VCC_ASSERT(cond->getType()->isIntegerTy() && "must be integer");
    cond = builder().CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0));
    builder().CreateCondBr(cond, while_true_block, fallthrough);

    builder().SetInsertPoint(while_true_block);
    for (Statement* statement : stmt->getStatements())
    {
        emitStatement(statement);
    }

    VCC_ASSERT(stmt->getStatements().size() >= 1 && "must be true for now");
    Statement* last_statement = stmt->getStatements()[stmt->getStatements().size() - 1];
    if (!isa<ReturnStatement>(last_statement))
        builder().CreateBr(cond_block);

    builder().SetInsertPoint(fallthrough);
}

void CodeGenerator::emitExternalDeclStatement(FunctionDecl* decl)
{
    llvm::FunctionType* function_type = decl->getFunctionType(context());
    m_function_map[decl] =
        llvm::Function::Create(function_type, llvm::Function::ExternalLinkage,
                               decl->getName(), m_context->getModule());
}

void CodeGenerator::emitAllocsStatement(FunctionDecl* decl)
{
    std::vector<DeclarationStatement*> declaration_statements{};

    for (Statement* statement : decl->getStatements())
    {
        switch (statement->getCode())
        {
            case code::DeclarationStatement:
            {
                declaration_statements.push_back(
                    statement->getAs<DeclarationStatement>());
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
        llvm::Type* llvm_type = statement->getType()->getType(context());
        llvm::Value* loc      = builder().CreateAlloca(llvm_type);
        symbolTable().insert(statement, statement->getName(), loc);
    }
}

void CodeGenerator::emitFunctionDeclStatement(FunctionDecl* decl)
{
    if (decl->isExtern())
        return emitExternalDeclStatement(decl);

    llvm::FunctionType* function_type = decl->getFunctionType(context());

    m_function_map[decl] =
        llvm::Function::Create(function_type, llvm::Function::ExternalLinkage,
                               decl->getName(), m_context->getModule());
    llvm::Function* llvm_func = getLLVMFunction(decl);
    llvm_func->setDSOLocal(true);


    llvm::BasicBlock* block = llvm::BasicBlock::Create(llvmContext(), "", llvm_func);
    builder().SetInsertPoint(block);

    emitFunctionArgListsStatement(decl->getArgList());
    emitAllocsStatement(decl);
    for (Statement* statement : decl->getStatements())
    {
        emitStatement(statement);
    }
}

// ======================================================
// ============= Expression emitters ===================

llvm::Value* CodeGenerator::emitConstantExpression(ConstantExpr* expr)
{
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext()),
                                  expr->getValue());
}

llvm::Value* CodeGenerator::emitCallExpression(CallExpr* expr)
{
    FunctionDecl* function_decl =
        m_context->getFunctionDeclTable().lookup(expr, expr->getFuncName()).value();

    VCC_ASSERT(function_decl && "this must exist for codegen!");
    VCC_ASSERT(function_decl->getFunctionType(context())->getNumParams() ==
                   expr->getExpressions().size() &&
               "expected the same number of argument");

    std::vector<llvm::Value*> args;
    int count = 0;
    for (auto it = function_decl->getArgBegin(), ie = function_decl->getArgsEnd();
         it != ie; ++it)
    {
        if (!Type::isSame(it->type, expr->getExpressions()[count]->getType(context())))
        {
            VCC_UNREACHABLE("implement getLine");
            // m_context->diagnostics.diag(expr, m_context->getLine(expr->getPos()),
            //                          "type mismatch");
            std::exit(-1);
        }

        args.push_back(emitExpression(expr->getExpressions()[count]));
        ++count;
    }

    if (count != static_cast<int>(expr->getExpressions().size()))
    {
        VCC_UNREACHABLE("implement getLine");
        // m_context->diagnostics.diag(expr, m_context->getLine(expr->getPos()),
        //                          "number of argument mismatch");
        std::exit(-1);
    }

    return builder().CreateCall(function_decl->getFunctionType(context()),
                                getLLVMFunction(function_decl), args);
}

llvm::Value* CodeGenerator::emitIntegerBinaryExpression(BinaryExpression* expr,
                                                        llvm::Value* left_hand_side,
                                                        llvm::Value* right_hand_side)
{
    VCC_ASSERT(right_hand_side->getType()->isIntegerTy() &&
               left_hand_side->getType()->isIntegerTy() &&
               "both side most be integer for now");

    if (right_hand_side->getType()->getPrimitiveSizeInBits() >
        left_hand_side->getType()->getPrimitiveSizeInBits())
    {
        left_hand_side = builder().CreateSExt(left_hand_side, right_hand_side->getType());
    }
    else if (left_hand_side->getType()->getPrimitiveSizeInBits() >
             right_hand_side->getType()->getPrimitiveSizeInBits())
    {
        right_hand_side =
            builder().CreateSExt(right_hand_side, left_hand_side->getType());
    }

    VCC_ASSERT(right_hand_side && left_hand_side && "cannot be null");
    VCC_ASSERT(left_hand_side->getType() == right_hand_side->getType());
    switch (expr->getKind())
    {
        case BinaryExpression::Add:
            return builder().CreateAdd(left_hand_side, right_hand_side);
        case BinaryExpression::Multiply:
            return builder().CreateMul(left_hand_side, right_hand_side);
        case BinaryExpression::Equal:
            return builder().CreateICmpEQ(left_hand_side, right_hand_side);
        case BinaryExpression::NEquals:
            return builder().CreateICmpNE(left_hand_side, right_hand_side);
        case BinaryExpression::GE:
            return builder().CreateICmpSGE(left_hand_side, right_hand_side);
        case BinaryExpression::GT:
            return builder().CreateICmpSGT(left_hand_side, right_hand_side);
        case BinaryExpression::Subtract:
            return builder().CreateSub(left_hand_side, right_hand_side);
        case BinaryExpression::LE:
            return builder().CreateICmpSLE(left_hand_side, right_hand_side);
        case BinaryExpression::LT:
            return builder().CreateICmpSLT(left_hand_side, right_hand_side);
        case BinaryExpression::Divide:
            VCC_ASSERT(left_hand_side->getType()->isIntegerTy() &&
                       right_hand_side->getType()->isIntegerTy());
            return builder().CreateUDiv(left_hand_side, right_hand_side);
        default:
            VCC_UNREACHABLE("cannot get here");
            return nullptr;
    }
}

llvm::Value* CodeGenerator::emitBinaryExpression(BinaryExpression* expr)
{
    llvm::Value* right_hand_side = emitExpression(expr->getRHS());
    llvm::Value* left_hand_side  = emitExpression(expr->getLHS());

    if (right_hand_side->getType()->isIntegerTy() &&
        left_hand_side->getType()->isIntegerTy())
        return emitIntegerBinaryExpression(expr, left_hand_side, right_hand_side);

    VCC_ASSERT((right_hand_side->getType()->isFloatingPointTy() ||
                left_hand_side->getType()->isFloatingPointTy()) &&
               "either side must be a floating point");

    if (!right_hand_side->getType()->isFloatingPointTy())
    {
        right_hand_side =
            builder().CreateSIToFP(right_hand_side, left_hand_side->getType());
    }
    else if (!left_hand_side->getType()->isFloatingPointTy())
    {
        VCC_ASSERT(left_hand_side->getType()->isIntegerTy());
        left_hand_side =
            builder().CreateSIToFP(left_hand_side, right_hand_side->getType());
    }

    VCC_ASSERT(right_hand_side && left_hand_side && "cannot be null");
    VCC_ASSERT(left_hand_side->getType() == right_hand_side->getType());
    switch (expr->getKind())
    {
        case BinaryExpression::Add:
            return builder().CreateFAdd(left_hand_side, right_hand_side);
        case BinaryExpression::Multiply:
            return builder().CreateFMul(left_hand_side, right_hand_side);
        case BinaryExpression::Equal:
            return builder().CreateFCmpOEQ(left_hand_side, right_hand_side);
        case BinaryExpression::NEquals:
            return builder().CreateFCmpONE(left_hand_side, right_hand_side);
        case BinaryExpression::GE:
            return builder().CreateFCmpOGE(left_hand_side, right_hand_side);
        case BinaryExpression::GT:
            return builder().CreateFCmpOGT(left_hand_side, right_hand_side);
        case BinaryExpression::Subtract:
            return builder().CreateFSub(left_hand_side, right_hand_side);
        case BinaryExpression::LE:
            return builder().CreateFCmpOLE(left_hand_side, right_hand_side);
        case BinaryExpression::LT:
            return builder().CreateFCmpOLT(left_hand_side, right_hand_side);
        case BinaryExpression::Divide:
            return builder().CreateFDiv(left_hand_side, right_hand_side);
        default:
            VCC_UNREACHABLE("cannot get here");
            return nullptr;
    }
}

llvm::Value* CodeGenerator::emitIdentifierExpression(IdentifierExpr* expr)
{
    llvm::Value* loc_value = symbolTable().lookup(expr, expr->getName()).value();
    Type* type             = expr->getType(context());
    return builder().CreateLoad(type->getType(context()), loc_value);
}

llvm::Value* CodeGenerator::emitStartOfPointerFromParentExpression(Expression* expression)
{
    if (MemberAccessExpression* member = dyncast<MemberAccessExpression>(expression))
        return emitCurrentRefMemberAccessExpression(member);

    if (DeRefExpression* ref = dyncast<DeRefExpression>(expression))
        return emitCurrentRefDeRefExpression(ref);

    return emitCurrentRefArrayAccessExpression(
        dyncast<ArrayAccessExpression>(expression));
}

llvm::Value* CodeGenerator::emitCurrentRefMemberAccessExpression(
    MemberAccessExpression* expr)
{
    llvm::Value* start_of_pointer =
        (expr->getParentExpression() == nullptr)
            ? symbolTable().lookup(expr, expr->getBaseName()).value()
            : emitStartOfPointerFromParentExpression(expr->getParentExpression());

    Type* current_type = expr->getGEPType(context());
    int field_num =
        current_type->getAs<StructType>()->getElement(expr->getMember())->field_num;
    llvm::Value* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext()), 0);
    llvm::Value* offset =
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext()), field_num);

    return builder().CreateGEP(current_type->getType(context()), start_of_pointer,
                               {zero, offset});
}

llvm::Value* CodeGenerator::emitCurrentRefArrayAccessExpression(
    ArrayAccessExpression* expr)
{
    llvm::Value* start_of_pointer =
        (expr->getParentExpression() == nullptr)
            ? symbolTable().lookup(expr, expr->getBaseName()).value()
            : emitStartOfPointerFromParentExpression(expr->getParentExpression());

    Type* type            = expr->getGEPType(context());
    llvm::Type* llvm_type = expr->getGEPType(context())->getType(context());
    if (!type->isArray())
        start_of_pointer = builder().CreateLoad(
            llvm::PointerType::get(llvmContext(), /*AddressSpace=*/0), start_of_pointer);

    llvm::ConstantInt* zero =
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmContext()), 0);
    llvm::Value* offset = emitExpression(expr->getIndexExpression());

    return builder().CreateGEP(llvm_type, start_of_pointer,
                               (type->isArray()) ? std::vector<llvm::Value*>{zero, offset}
                                                 : std::vector<llvm::Value*>{offset});
}

llvm::Value* CodeGenerator::emitCurrentRefDeRefExpression(DeRefExpression* expr)
{
    VCC_ASSERT(expr->getRef()->getType(context())->isPointer());
    return emitExpression(expr->getRef());
}

llvm::Value* CodeGenerator::emitMemberAccessExpression(MemberAccessExpression* expr)
{
    if (expr->getChildPosfixExpression())
        return emitExpression(expr->getChildPosfixExpression());

    llvm::Value* ref_loc   = emitCurrentRefMemberAccessExpression(expr);
    llvm::Type* child_type = expr->getGEPType(context())
                                 ->getAs<StructType>()
                                 ->getElement(expr->getMember())
                                 ->type->getType(context());
    return builder().CreateLoad(child_type, ref_loc);
}

llvm::Value* CodeGenerator::emitArrayAccessExpression(ArrayAccessExpression* expr)
{
    if (expr->getChildPosfixExpression())
        return emitExpression(expr->getChildPosfixExpression());

    llvm::Value* start_of_pointer = emitCurrentRefArrayAccessExpression(expr);
    Type* current_type            = expr->getGEPType(context());
    Type* child_type =
        current_type->isBuiltin() ? current_type : expr->getGEPChildType(context());
    return builder().CreateLoad(child_type->getType(context()), start_of_pointer);
}

llvm::Value* CodeGenerator::emitDeRefExpression(DeRefExpression* expr)
{
    if (expr->getPosfixChild())
        return emitExpression(expr->getPosfixChild());

    VCC_ASSERT(expr->getRef()->getType(context())->isPointer());
    llvm::Value* current_value = emitExpression(expr->getRef());
    llvm::Type* base_type =
        expr->getRef()->getType(context())->getAs<PointerType>()->getPointee()->getType(
            context());

    return builder().CreateLoad(base_type, current_value);
}

llvm::Value* CodeGenerator::emitRefExpression(RefExpression* expr)
{
    return emitRefExpression(dyncast<LocatorExpression>(expr->getInnerExpression()));
}

llvm::Value* CodeGenerator::emitStringLiteralExpression(StringLiteral* expr)
{
    return builder().CreateGlobalString(expr->getString());
}

llvm::Value* CodeGenerator::emitBuiltinCastExpression(CastExpression* expr,
                                                      BuiltinType* from, BuiltinType* to)
{
    VCC_ASSERT(from && to);
    VCC_ASSERT(!Type::isSame(from, to));

    if (from->isIntegerKind() && to->isFloat())
    {
        llvm::Value* value = emitExpression(expr->getCastedExpression());
        return builder().CreateSIToFP(value, to->getType(context()));
    }

    if (from->isFloat() && to->isIntegerKind())
    {
        llvm::Value* value = emitExpression(expr->getCastedExpression());
        return builder().CreateFPToSI(value, to->getType(context()));
    }

    VCC_ASSERT(from->isIntegerKind() && to->isIntegerKind());
    VCC_ASSERT(from->getBitSize() != to->getBitSize());

    if (from->isBool())
    {
        llvm::Value* val = emitExpression(expr->getCastedExpression());
        return builder().CreateZExt(val, to->getType(context()));
    }

    if (from->getBitSize() > to->getBitSize())
    {
        llvm::Value* val = emitExpression(expr->getCastedExpression());
        return builder().CreateTrunc(val, to->getType(context()));
    }

    llvm::Value* val = emitExpression(expr->getCastedExpression());
    return builder().CreateSExt(val, to->getType(context()));
}

llvm::Value* CodeGenerator::emitCastExpression(CastExpression* expr)
{
    Type* from_type = expr->getCastedExpression()->getType(context());
    if (Type::isSame(from_type, expr->getCastTo()))
        return emitExpression(expr->getCastedExpression());

    if (from_type->isBuiltin() && expr->getCastTo()->isBuiltin())
        return emitBuiltinCastExpression(expr, from_type->getAs<BuiltinType>(),
                                         expr->getCastTo()->getAs<BuiltinType>());

    if ((from_type->isPointer() && expr->getCastTo()->isVoidPtr()) ||
        (from_type->isVoidPtr() && expr->getCastTo()->isPointer()))
        return emitExpression(expr->getCastedExpression());

    VCC_UNREACHABLE("implmement getLine");
    // m_context->diagnostics.diag(expr, m_context->getLine(expr->getPos()),
    //                          "cannot perform a cast");
    std::exit(-1);
    return nullptr;
}

// ======================================================
// ============= Ref expression emitters ===============

llvm::Value* CodeGenerator::emitRefIdentifierExpression(IdentifierExpr* expr)
{
    return symbolTable().lookup(expr, expr->getName()).value();
}

llvm::Value* CodeGenerator::emitRefMemberAccessExpression(MemberAccessExpression* expr)
{
    if (expr->getChildPosfixExpression())
        return emitRefExpression(expr->getChildPosfixExpression());

    return emitCurrentRefMemberAccessExpression(expr);
}

llvm::Value* CodeGenerator::emitRefArrayAccessExpression(ArrayAccessExpression* expr)
{
    if (expr->getChildPosfixExpression())
        return emitRefExpression(expr->getChildPosfixExpression());

    return emitCurrentRefArrayAccessExpression(expr);
}

llvm::Value* CodeGenerator::emitRefDeRefExpression(DeRefExpression* expr)
{
    if (expr->getPosfixChild())
        return emitRefExpression(expr->getPosfixChild());

    VCC_ASSERT(expr->getRef()->getType(context())->isPointer());
    return emitExpression(expr->getRef());
}

llvm::Value* CodeGenerator::emitRefRefExpression(RefExpression* expr)
{
    VCC_UNREACHABLE("this is ill form");
    return emitRefExpression(dyncast<LocatorExpression>(expr->getInnerExpression()));
}

llvm::Function* CodeGenerator::getLLVMFunction(const FunctionDecl* decl) const
{
    auto it = m_function_map.find(decl);
    VCC_ASSERT(it != m_function_map.end() && "function not registered in codegen");
    return it->second;
}

// ======================================================
// ============= Nice to have functions ===================
ContextHolder CodeGenerator::context()
{
    return m_context;
}

llvm::IRBuilder<>& CodeGenerator::builder()
{
    return m_context->getBuilder();
}

llvm::LLVMContext& CodeGenerator::llvmContext()
{
    return m_context->getContext();
}

CGSymbolTable& CodeGenerator::symbolTable()
{
    return m_symbol_table;
}
