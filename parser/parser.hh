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
	
class Expression
{
  public:
    Expression() {}
};

class Program
{
  protected:
    std::unique_ptr<Statement> statements;

  public:
    Program() {}
};

class Parser
{

  protected:
    std::unique_ptr<Lexer> lexer;

  public:
    Parser() {}
};
}

#endif
