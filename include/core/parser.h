#ifndef CORE_PARSER_H
#define CORE_PARSER_H

#include "core/ast.h"
#include "core/context.h"
#include "core/lex.h"
#include "core/scope.h"
#include "core/sema.h"
#include "core/type.h"

namespace vcc
{
class Parser
{
   public:
    Parser(ContextHolder context);

    void start();
    const std::vector<Statement*>& getSyntaxTree();
    ContextHolder getHolder();
    bool haveError() const;

   private:
    /// The following are function used by the parser
    /// to makes things look pretty.
    /// Get the current tokenType with consuming it.
    lex::TokenType tok();

    /// If the next tokens are the same as the list, consume
    /// the tokens (with the exception when consumeLast is set).
    /// Otherwise reset to the beginning as as if nothing has happened
    bool at(lex::TokenType token);
    bool at(std::initializer_list<lex::TokenType> list, bool consume_last = false);

    /// Return true if the current token has this type
    bool is(lex::TokenType type);

    /// Getting the string of the identifier. You must check
    /// that the current token is an identifier. If not, the
    /// program will likely crash
    std::string getTokenString();
    int getIntegerLiteral();

    /// Consume one token
    void consume();

    const std::vector<Statement*>& buildSyntaxTree();

    // Types, kind of like statements but not necessary
    // return a pointer when success, nullptr otherwise
    Type* buildTypeQualification();
    void addStructDefinition();

    // building the function decl
    Statement* buildFunctionDecl();
    FunctionArgLists* buildFunctionArgList();

    // Statements
    Statement* buildAssignmentStatement();
    Statement* buildReturnStatement();
    Statement* buildStatement();
    Statement* buildIfStatement();
    Statement* buildWhileStatement();
    Statement* buildDeclarationStatement();
    Statement* buildExternalDecl();
    Statement* buildCallStatement();

    // Expressions
    Expression* buildExpression();
    Expression* buildRefExpression();
    Expression* buildCastExpression();
    Expression* buildBinaryExpression(int min_precedence);
    Expression* buildTrivialExpression();
    Expression* buildDerefExpression();
    Expression* buildCallExpr();
    LocatorExpression* buildPosfixExpression(LocatorExpression* lhs = nullptr);
    LocatorExpression* buildTailPosfixExpression(
        LocatorExpression* lhs);  // helper for above


    Scope *m_root_scope, *m_current_scope;
    Scope* getCurScope();
    /// Scope RAIL is a nice wrapper behind function that defines a
    /// scope. The user of the parser class can now just call getScope
    /// which returns the scope class which is needed to add symbol into
    /// the symbol table
    struct ScopeRAIL
    {
        ScopeRAIL(Parser& parser);
        ~ScopeRAIL();

        Parser& parser;
    };

    /// This is a way so that we can use one interface to log all the error
    /// The problem we are trying to solve is that logError have to return a
    /// type that is compatible to Expression , Type*, and Statement*.
    struct ErrorResult
    {
        inline operator Expression*()
        {
            return nullptr;
        }
        inline operator Statement*()
        {
            return nullptr;
        }
        inline operator Type*()
        {
            return nullptr;
        }
    };
    inline ErrorResult logError(const std::string& message);

    // for binary expression
    // clang-format off
    const static inline std::unordered_map<lex::TokenType, int> precedence_level={
      // eq, ne, gt, ge, lt, le
      {lex::EqualKeyword, 1},
      {lex::NEquals, 1},
      {lex::GreaterThan, 1},
      {lex::GreaterEqual, 1},
      {lex::LessThan, 1},
      {lex::LessEqual, 1},

      {lex::Add, 2},
      {lex::Subtract, 2},

      {lex::Multiply, 3},
      {lex::Divide, 3},
    };
    // clang-format on

    ContextHolder m_context;
    lex::Tokenizer m_tokenizer;

    // Store the computation results
    std::vector<Statement*> m_top_level_statements;
    std::unordered_map<std::string, StructType*> m_struct_defs;
};
};  // namespace vcc

#endif
