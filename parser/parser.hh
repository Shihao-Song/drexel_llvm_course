#ifndef __PARSER_HH__
#define __PARSER_HH__

#include "lexer/lexer.hh"

#include <memory>
#include <variant>

namespace Frontend
{
// set statement
//     set <identifier> = <expression>;
// Test 1:
//     set x = 5;
//     set y = 10;
//     set z = 6.5;

class Identifier
{
  protected:
    Token tok;

  public:
    Identifier() {}

    Identifier(const Identifier &_iden) : tok(_iden.tok) {}

    Identifier(Token &_tok) : tok(_tok) {}

    auto& getLiteral() { return tok.getLiteral(); }
};

class Expression
{
  protected:
    Token tok;

  public:
    std::variant<int, float> literal;
    
  public:
    Expression() {}

    Expression(const Expression &_expr) : tok(_expr.tok) {}

    Expression(Token &_tok) : tok(_tok) {}
};

enum class StatementType : int
{
    SET_STATEMENT,
    ILLEGAL
};
class Statement
{
  protected:
    StatementType type = StatementType::ILLEGAL;

  public:
    Statement() {}
};
class SetStatement : public Statement
{
  public:
    SetStatement() 
        : Statement()
    {
        type = StatementType::SET_STATEMENT;
    }
};

class Program
{
  protected:
    std::vector<std::unique_ptr<Statement>> statements;

  public:
    Program() {}

    void addStatement(std::unique_ptr<Statement> &_statement)
    {
        statements.push_back(std::move(_statement));
    }
};

class Parser
{
  protected:
    Program program;

  protected:
    Token cur_token;
    Token next_token;    

  protected:
    std::unique_ptr<Lexer> lexer;

  public:
    Parser(const char* fn); 

  protected:
    void parseProgram();
    void advanceTokens();

    std::unique_ptr<Statement> parseSetStatement();
    Expression parseExpression();
};
}

#endif
