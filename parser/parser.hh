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

/* Identifier definition */
class Identifier
{
  protected:
    Token tok;

  public:
    Identifier() {}

    Identifier(const Identifier &_iden) : tok(_iden.tok) {}

    Identifier(Token &_tok) : tok(_tok) {}

    virtual std::string print()
    {
        std::string ret = "{ ";
        ret += ("type: " + tok.prinTokenType() + ", ");
        ret += ("value: " + tok.getLiteral() + " }");

        return ret;
    }

    auto &getLiteral() { return tok.getLiteral(); }
    auto getType() { return tok.prinTokenType(); }
};

/* Expression definition */
enum class ExpressionType : int
{
    LITERAL,
    ILLEGAL
};
class Expression
{
  protected:
    ExpressionType type = ExpressionType::ILLEGAL;
    
  public:
    Expression() {}

    virtual std::string print(unsigned level) { return "[Error] No implementation"; }
};
class LiteralExpression : public Expression
{
  protected:
    Token tok;
    
  public:
    LiteralExpression() 
    {
        type = ExpressionType::LITERAL;
    }

    LiteralExpression(const LiteralExpression &_expr) 
    {
        tok = _expr.tok;
        type = ExpressionType::LITERAL;
    }

    LiteralExpression(Token &_tok) : tok(_tok) 
    {
        type = ExpressionType::LITERAL;
    }

    std::string print(unsigned level) override;
};

/* Statement definition*/
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

    virtual void printStatement() {}
};
class SetStatement : public Statement
{
  protected:
    std::unique_ptr<Identifier> iden;
    std::unique_ptr<Expression> expr;

  public:
    SetStatement() 
    {
        type = StatementType::SET_STATEMENT;
    }

    SetStatement(std::unique_ptr<Identifier> &_iden,
                 std::unique_ptr<Expression> &_expr)
    {
        type = StatementType::SET_STATEMENT;

        iden = std::move(_iden);
        expr = std::move(_expr);
    }
    
    void printStatement() override;
};

/* Program definition */
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

    void printStatements()
    {
        for (auto &statement : statements) { statement->printStatement(); }
    }
};

/* Parser definition */
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

    void printStatements() { program.printStatements(); }

  protected:
    void parseProgram();
    void advanceTokens();

    std::unique_ptr<Statement> parseSetStatement();
    std::unique_ptr<Expression> parseExpression();
};
}

#endif
