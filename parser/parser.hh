#ifndef __PARSER_HH__
#define __PARSER_HH__

#include "lexer/lexer.hh"

#include <memory>

namespace Frontend
{
typedef std::unique_ptr<Lexer> LexerPtr;
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
typedef std::unique_ptr<Statement> StatementPtr;
	
class Expression
{
  public:
    Expression() {}
};

class Program
{
  protected:
    std::vector<StatementPtr> statements;

  public:
    Program() {}
};
typedef std::unique_ptr<Program> ProgramPtr;

class Parser
{
  protected:
    ProgramPtr program;

  protected:
    Token cur_token;
    Token next_token;    

  protected:
    LexerPtr lexer;

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
