#include "parser.h"
#include "ast.h"
#include <cassert>
#include <iostream>

using lex::Token;

Parser::Parser(const char *filename, ContextHolder context)
    : m_tokenizer(filename), m_context(context) {}

void Parser::start(){
    static bool started_before = false;
    if(!started_before){
        buildSyntaxTree();
        started_before = true;
    }
}

const std::vector<ASTBase*>& Parser::getSyntaxTree(){ return m_function_decls; }

// type_qualification :== 'int' {'*'} | 'struct', <identifier> |
//                     'array', '(', <integer_literal>, ')', <type_qualification>

Type* Parser::buildTypeQualification(){
    // we have an array type
    // 'array', '(', <integer_literal>, ')', <type_qualification>
    if(m_tokenizer.getCurrentType() == lex::Array){
        if(m_tokenizer.getNextType() != lex::LeftParentheses){
            logError("expectex (");
            return nullptr;
        }
        m_tokenizer.consume();

        if(m_tokenizer.getCurrentType() != lex::IntegerLiteral){
            logError("expected integer");
            return nullptr;
        }
        int count = m_tokenizer.current().getIntegerLiteral();
        m_tokenizer.consume();

        if(m_tokenizer.getCurrentType() != lex::RightParentheses){
            logError("expected )");
            return nullptr; 
        }
        m_tokenizer.consume();

        Type* base = buildTypeQualification();
        return new ArrayType(base, count);
    }

    // we have builtin
    if(m_tokenizer.getCurrentType() == lex::Int){
        m_tokenizer.consume();
        if(m_tokenizer.peek().getType() == lex::Multiply){
            m_tokenizer.consume();
            return new PointerType(new BuiltinType(BuiltinType::Int)); 
        }

        return new BuiltinType(BuiltinType::Int);
    }

    // we have a structure
    if(m_tokenizer.getCurrentType() == lex::Struct){
        m_tokenizer.consume();
        if(m_tokenizer.getCurrentType() != lex::Identifier){
            logError("expected identifier");
            return nullptr;
        }

        std::string struct_name = m_tokenizer.current().getStringLiteral();
        m_tokenizer.consume();

        assert(m_struct_defs.contains(struct_name) && "undefined reference to struct");
        return m_struct_defs[struct_name];
    }

    return nullptr;
}

// struct_definition :== 'struct', <identifier> ,'{'
//                  , {<type_qualification> <identifier>}+, '}'
void Parser::addStructDefinition(){
    if(m_tokenizer.getCurrentType() != lex::Struct){
        logError("expected struct");
        return;
    }

    if(m_tokenizer.getNextType() != lex::Identifier){
        logError("expected identifier");
        return;
    }
    std::string name = m_tokenizer.current().getStringLiteral();

    if(m_tokenizer.getNextType() != lex::LeftBrace){
        logError("expected {");
        return;
    }
    m_tokenizer.consume();

    // parsing the struct
    std::vector<StructType::Element> elements; 
    int element_count  =  0;
    while(Type* current = buildTypeQualification()){
        if(m_tokenizer.getCurrentType() != lex::Identifier){
            logError("expected identifier");
            return;
        }

        // FIXME: we need to check that there are no duplicated name
        std::string name = m_tokenizer.current().getStringLiteral();
        elements.push_back({element_count, name, current});
        if(m_tokenizer.getNextType() != lex::Comma){
            logError("expected ,");
            return;
        }
        m_tokenizer.consume();

        ++element_count;
    }

    if(m_tokenizer.getCurrentType() != lex::RightBrace){
        logError("expected }");
        return;
    }

    m_tokenizer.consume();

    // Finally, inserting the element into the table
    assert(!m_struct_defs.contains(name) && "we have duplicated definition of the same struct");
    m_struct_defs[name] = new StructType(elements, name);
}

// top_level :== <function_decl> | <struct_definition>
const std::vector<ASTBase*>& Parser::buildSyntaxTree() {
    assert(m_function_decls.size() == 0 && "can only be called once");
    while(m_tokenizer.getCurrentType() != lex::EndOfFile){
        if(m_tokenizer.getCurrentType() == lex::FunctionDecl){
            ASTBase* base = buildFunctionDecl();
            m_function_decls.push_back(base);
            continue;
        }

        if(m_tokenizer.getCurrentType() == lex::Struct){
            addStructDefinition();
            continue;
        }

        break;
    }

    if(m_tokenizer.getCurrentType() != lex::EndOfFile){
        logError("cannot parse things starting here");
    }

    return m_function_decls;
}


ASTBase * Parser::logError(const char *message) {
    m_error = true;

    Token current_token = m_tokenizer.current();
  std::cerr << current_token.getPos().row << ":" << current_token.getPos().col << " Error: " << message << "\n";
  std::string line = m_tokenizer.getLine(current_token.getPos());
  std::cerr <<  line << "\n";
  for(int i = 0;i<current_token.getPos().col-1;++i){
      std::cerr << " ";
  }
  std::cerr << "^---see here. \n" << std::endl;

  return nullptr;
}

// if_statement :== 'if', <expression>, 'then', <statements>+, 'end'
ASTBase* Parser::buildIfStatement(){
    if(m_tokenizer.getCurrentType() != lex::If)
        return logError("expected if");
    m_tokenizer.consume();

    ASTBase* cond = buildExpression();

    if(m_tokenizer.getCurrentType() != lex::Then)
        return logError("expected then");
    m_tokenizer.consume();

    std::vector<ASTBase*> expressions;
    while(ASTBase* expression = buildStatement()){
        expressions.push_back(expression);
    }

    if(m_tokenizer.getCurrentType() != lex::End)
        return logError("expected end");
    m_tokenizer.consume();

    return new IfStatement(cond, std::move(expressions));
}

// while_statement :== 'while', <expression> 'then', <statements>+,'end'
ASTBase* Parser::buildWhileStatement(){
    if(m_tokenizer.getCurrentType() != lex::While)
        return logError("expected while");
    m_tokenizer.consume();

    ASTBase* cond = buildExpression();

    if(m_tokenizer.getCurrentType() != lex::Then)
        return logError("expected then");
    m_tokenizer.consume();

    std::vector<ASTBase*> expressions {};
    while(ASTBase* statement = buildStatement())
        expressions.push_back(statement);
        
    if(m_tokenizer.getCurrentType() != lex::End)
        return logError("expected end");
    m_tokenizer.consume();

    return new WhileStatement(cond, std::move(expressions));
}

// statements :== <assignment_statement> | <return_statement> | <if_statement> | <while_statement>
ASTBase *Parser::buildStatement() {
    if(m_tokenizer.getCurrentType() == lex::If)
        return buildIfStatement();

  if (m_tokenizer.getCurrentType() == lex::Ret)
    return buildReturnStatement();

  if(m_tokenizer.current().isTypeQualification())
      return buildDeclarationStatement();

  if(m_tokenizer.getCurrentType() == lex::While)
      return buildWhileStatement();

  return buildAssignmentStatement();
}

// function_decl :== 'function', <identifier>, 'gives', <type_qualification>,
//                     <function_args_list>, '{', <expression>+, ''}'
ASTBase *Parser::buildFunctionDecl() {
  if (m_tokenizer.current().getType() != lex::FunctionDecl)
    return logError("function declaration must begin with keyword function");

  // eat function decl
  Token name_token = m_tokenizer.next();
  if (name_token.getType() != lex::Identifier)
    return logError("function declaration does not have identifier");

  std::string name = name_token.getStringLiteral();

  if (m_tokenizer.getNextType() != lex::Gives)
    return logError("function declaration must provide return type");
  m_tokenizer.consume();

  Type* return_type = buildTypeQualification();

  FunctionArgLists *arg_list = buildFunctionArgList();

  if (m_tokenizer.getCurrentType() != lex::LeftBrace)
    return logError("expected {");
  m_tokenizer.consume();

  // currently only assignment expression is supported
  ASTBase *exp;
  std::vector<ASTBase *> expressions;
  while ((exp = buildStatement())) {
    assert(exp && "expression must be non nullptr");
    expressions.push_back(exp);
  }

  if (m_tokenizer.getCurrentType() != lex::RightBrace)
    return logError("expected }");
  m_tokenizer.consume();

  return new FunctionDecl(
      expressions, dynamic_cast<FunctionArgLists *>(arg_list), std::move(name), return_type);
}

// assignment_expression :== <identifier>, '=', <expression>, ';'
ASTBase *Parser::buildAssignmentStatement() {
  if (m_tokenizer.getCurrentType() != lex::Identifier)
    return nullptr;

  assert(m_tokenizer.getCurrentType() == lex::Identifier);
  std::string name = m_tokenizer.current().getStringLiteral();
  if (m_tokenizer.getNextType() != lex::Equal)
    return logError("expected =");
  m_tokenizer.consume();

  ASTBase *expression = buildExpression();

  if (m_tokenizer.getCurrentType() != lex::SemiColon)
    return logError("expected semi colon");

  m_tokenizer.consume();

  return new AssignmentStatement(name, expression);
}

// FIXME: maybe put arg_declaration into its own function?
// function_args_list :== '[', args_declaration+, ']'
//  args_declaration :== <type_qualification> + identifier + ','
FunctionArgLists *Parser::buildFunctionArgList() {
  if (m_tokenizer.getCurrentType() != lex::LeftBracket) {
    logError("expected [");
    return nullptr;
  }
  m_tokenizer.consume();

  // parsing args declaration
  // FIXME: add a way to map token into type qualification
  std::vector<TypeInfo> args{};
  while (m_tokenizer.current().isTypeQualification()) {
    Type* type = buildTypeQualification();
    lex::Token next_token = m_tokenizer.current();

    if (next_token.getType() != lex::Identifier) {
      logError("expected identifier");
      return nullptr;
    }
    std::string name = next_token.getStringLiteral();

    if (m_tokenizer.getNextType() != lex::Comma) {
      logError("expected comma");
      return nullptr;
    }
    args.push_back(TypeInfo{type, name});
    m_tokenizer.consume();
  }

  if (m_tokenizer.getCurrentType() != lex::RightBracket) {
    logError("expected ]");
    return nullptr;
  }

  // pop this token ]
  m_tokenizer.consume();

  return new FunctionArgLists(std::move(args));
}

// return_statement :== 'ret', <expression> ';'
ASTBase *Parser::buildReturnStatement() {
  if (m_tokenizer.getCurrentType() != lex::Ret) {
    return logError("expected error");
  }
  m_tokenizer.consume();

  ASTBase *expression = buildExpression();

  if (m_tokenizer.getCurrentType() != lex::SemiColon) {
    return logError("expected ;");
  }
  m_tokenizer.consume();

  return new ReturnStatement(expression);
}

// expression :==  <binary_expression>
ASTBase *Parser::buildExpression() {
  return buildBinaryExpression(/*min_precedence=*/1);
}

// binary_expression :== <trivial_expression> |
//                             <trivial_expression>, <bin_op>,
//                             <binary_expression>
ASTBase *Parser::buildBinaryExpression(int min_precendence) {
  // precedence climbing based parsing:
  // https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing
  // result = compute_atom()

  // while cur token is a binary operator with precedence >= min_prec:
  //     prec, assoc = precedence and associativity of current token
  //     if assoc is left:
  //         next_min_prec = prec + 1
  //     else:
  //         next_min_prec = prec
  //     rhs = compute_expr(next_min_prec)
  //     result = compute operator(result, rhs)
  //
  // return result

  ASTBase *result = buildTrivialExpression();
  assert(result && "not sure how can this be false?");

  // this is just a trivial expression case
  Token current_operator_token = m_tokenizer.current();
  if (!current_operator_token.isBinaryOperator()) {
    return result;
  }

  int current_precedence_level =
      precedence_level.find(current_operator_token.getType())->second;
  while (current_precedence_level >= min_precendence) {
    current_operator_token = m_tokenizer.current();
    // at the beginning of every loop iteration, the current token
    // must be a binary operator. If this is not a binary operator, it is likely
    // that
    // we are done
    if (!current_operator_token.isBinaryOperator())
      break;

    current_precedence_level =
        precedence_level.find(current_operator_token.getType())->second;

    result = new BinaryExpression(
        result, BinaryExpression::getFromLexType(current_operator_token));
    m_tokenizer.consume(); // consume the binary token

    int next_precedence_level = current_precedence_level + 1;

    ASTBase *rhs = buildBinaryExpression(next_precedence_level);
    dynamic_cast<BinaryExpression *>(result)->setRHS(rhs);
  }

  return result;
}

ASTBase* Parser::buildMemberAccessExpression(){
    if(m_tokenizer.getCurrentType() != lex::Identifier){
        return logError("expected identifier");
    }

    std::vector<std::string> identifiers; 
    while(m_tokenizer.getCurrentType() == lex::Identifier 
            && m_tokenizer.peek().getType() == lex::Fullstop){
        identifiers.push_back(m_tokenizer.current().getStringLiteral());

        // we have already checked for bot full stop as well from the while statement 
        // from above
        m_tokenizer.consume();
        m_tokenizer.consume();
    }

    identifiers.push_back(m_tokenizer.current().getStringLiteral());
    m_tokenizer.consume(); // consumes the final token

    return new MemberAccessExpression (identifiers);

}

// trivial_expression :== <identifier> | <call_expression> |
//                             '(', <expression>, ')' | <integer_literal>  | <member_access_expression>
ASTBase *Parser::buildTrivialExpression() {
  // FIXME: add call expression
  // <integer_literal>
  if (m_tokenizer.getCurrentType() == lex::IntegerLiteral) {
    ASTBase *value =
        new ConstantExpr(m_tokenizer.current().getIntegerLiteral());
    m_tokenizer.consume();
    return value;
  }

  // <identifier>
  if (m_tokenizer.getCurrentType() == lex::Identifier) {
    // <identifier>  + '(' means call expr
    if (m_tokenizer.peek().getType() == lex::LeftParentheses)
      return buildCallExpr();
    if(m_tokenizer.peek().getType() == lex::Fullstop)
        return buildMemberAccessExpression();

    ASTBase *value =
        new IdentifierExpr(m_tokenizer.current().getStringLiteral());
    m_tokenizer.consume();

    return value;
  }

  // '(', <expression> , ')'
  if (m_tokenizer.getCurrentType() == lex::LeftParentheses) {
    m_tokenizer.consume(); // consume ; )
    ASTBase *value = buildExpression();

    if (m_tokenizer.getCurrentType() != lex::RightParentheses) {
      return logError("expected )");
    }

    m_tokenizer.consume();
    return value;
  }

  return logError("cannot build trivial expression!");
}

// call_expressions :== <identifier>, '(', { <expression> ',' }+,  ')'
ASTBase *Parser::buildCallExpr() {
  if (m_tokenizer.getCurrentType() != lex::Identifier)
    return logError("expected identfier");

  std::string function_name = m_tokenizer.current().getStringLiteral();

  if (m_tokenizer.getNextType() != lex::LeftParentheses)
    return logError("expected (");
  m_tokenizer.consume();

  std::vector<ASTBase *> expressions;

  // {<expression>, ','} +
  while (m_tokenizer.getCurrentType() != lex::RightParentheses) {
    ASTBase *expression = buildExpression();
    expressions.push_back(expression);

    // consume the comma
    if (m_tokenizer.getCurrentType() != lex::Comma)
      return logError("expected ," );
    m_tokenizer.consume();
  }

  if (m_tokenizer.getCurrentType() != lex::RightParentheses)
    return logError("expected )");
  m_tokenizer.consume();

  return new CallExpr(function_name, expressions);
}

ContextHolder Parser::getHolder(){
    return m_context;
}

bool Parser::haveError()const{
    return m_error;
}

// declaration_statement :== <type_qualification>, <identifier>, {'=', <expression>} , ';'
ASTBase* Parser::buildDeclarationStatement(){
    Type* parsed_type = buildTypeQualification();

    if (m_tokenizer.getCurrentType() != lex::Identifier)
        return logError("expected identifier");

    std::string name = m_tokenizer.current().getStringLiteral();

    //  we have this case 
    // <type_qualification>, <identifier>, ';'
    if(m_tokenizer.getNextType() == lex::SemiColon){
        m_tokenizer.consume(); // applying the side effect
        return new DeclarationStatement(name, nullptr, parsed_type);
    }
    
    if(m_tokenizer.getCurrentType() != lex::Equal)
        return logError("expected =");
    m_tokenizer.consume();

    ASTBase* expression = buildExpression();

    if(m_tokenizer.getCurrentType() != lex::SemiColon)
        return logError("expected ;");
    m_tokenizer.consume();

    return new DeclarationStatement(name, expression, parsed_type);
}
