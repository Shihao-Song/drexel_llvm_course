#ifndef __PARSER_HH__
#define __PARSER_HH__

#include "lexer/lexer.hh"

#include <memory>

namespace Frontend
{
// set statement
//     set <identifier> = <expression>;
// Test 1:
//     set x = 5;
//     set y = 10;
//     set z = 6.5;
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

class Expression
{
  public:
    Expression() {}
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
    Parser(const char* fn) 
        : lexer(new Lexer(fn))
    {
        parseProgram();
    }

  protected:
    void parseProgram();
    bool advanceTokens();
};
}

#endif
