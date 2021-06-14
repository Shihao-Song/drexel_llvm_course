#ifndef __PARSER_HH__
#define __PARSER_HH__

#include "lexer/lexer.hh"

#include <iostream>
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
        return tok.getLiteral();
    }

    auto &getLiteral() { return tok.getLiteral(); }
    auto getType() { return tok.prinTokenType(); }
};

/* Expression definition */
class Expression
{
  public:
    enum class ExpressionType : int
    {
        LITERAL,
        PLUS,
        MINUS,
        ASTERISK,
        SLASH,
        ILLEGAL
    };

  protected:

    ExpressionType type = ExpressionType::ILLEGAL;
    
  public:
    Expression() {}

    auto getType() { return type; }

    virtual std::string getLiteral() { return "[Error] No implementation"; }
    virtual std::string print(unsigned level) { return "[Error] No implementation"; }
    virtual std::string printExpr() { return "[Error] No implementation"; } 
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

    std::string getLiteral() { return tok.getLiteral(); }

    std::string printExpr() override
    {
        return tok.getLiteral();
    }

    std::string print(unsigned level) override
    {
        return (tok.getLiteral() + "\n");
    }
};

class ArithExpression : public Expression
{
  protected:
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;

  public:

    ArithExpression(std::unique_ptr<Expression> &_left,
                    std::unique_ptr<Expression> &_right,
                    ExpressionType _type)
    {
        left = std::move(_left);
        right = std::move(_right);
        type = _type;
    }

    ArithExpression(const ArithExpression &_expr)
    {
        left = std::move(_expr.left);
        right = std::move(_expr.right);
        type = _expr.type;
    }

    std::string print(unsigned level) override
    {
        std::string prefix(level * 2, ' ');

        std::string ret = "";
        if (left != nullptr)
        {
            if (left->getType() == ExpressionType::LITERAL)
            {
                ret += prefix;
            }

            ret += left->print(level + 1);
        }
        
        if (right != nullptr)
        {
            ret += prefix;
            if (type == ExpressionType::PLUS)
            {
                ret += "+";
            }
            else if (type == ExpressionType::MINUS)
            {
                ret += "-";
            }
            else if (type == ExpressionType::ASTERISK)
            {
                ret += "*";
            }
            else if (type == ExpressionType::SLASH)
            {
                ret += "/";
            }
            ret += "\n";
            
            if (right->getType() == ExpressionType::LITERAL)
            {
                ret += prefix;
            }

            ret += right->print(level + 1);
        }
         
        return ret;
    }
    
    std::string printExpr() override
    {
        std::string ret = "";
        if (left != nullptr)
        {
            if (left->getType() == ExpressionType::LITERAL)
            {
                ret += left->getLiteral();
            }
            else
            {
                ret += left->printExpr();
            }
        }
        
        if (right != nullptr)
        {
            if (type == ExpressionType::PLUS)
            {
                ret += " + ";
            }
            else if (type == ExpressionType::MINUS)
            {
                ret += " - ";
            }
            else if (type == ExpressionType::ASTERISK)
            {
                ret += " * ";
            }
            else if (type == ExpressionType::SLASH)
            {
                ret += " / ";
            }
            
            if (right->getType() == ExpressionType::LITERAL)
            {
                ret += right->getLiteral();
            }
            else
            {
                ret += right->printExpr();
            }
        }
         
        return ret;
    }

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
        std::cout << "\n\nExpressions: \n";

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
    std::unique_ptr<Expression> parseTerm();
    std::unique_ptr<Expression> parseFactor();
};
}

#endif
