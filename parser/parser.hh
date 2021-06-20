#ifndef __PARSER_HH__
#define __PARSER_HH__

#include "lexer/lexer.hh"

#include <iostream>
#include <memory>
#include <variant>

// LLVM IR codegen libraries
// TODO - shihao, I think we should have a separate codegen class
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

namespace Frontend
{
/* Identifier definition */
class Identifier
{
  protected:
    Token tok;

  public:
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
        LITERAL, // i.e., 1

        PLUS, // i.e., 1 + 2
        MINUS, // i.e., 1 - 2
        ASTERISK, // i.e., 1 * 2
        SLASH, // i.e., 1 / 2

        CALL, // TODO-shihao

        ILLEGAL
    };

  protected:

    ExpressionType type = ExpressionType::ILLEGAL;
    
  public:
    Expression() {}

    auto getType() { return type; }

    virtual std::string getLiteral() { return "[Error] No implementation"; }
    virtual std::string print(unsigned level) { return "[Error] No implementation"; }
};

class LiteralExpression : public Expression
{
  protected:
    Token tok;
    
  public:
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

    // Debug print associated with the print in ArithExp
    std::string print(unsigned level) override
    {
        return (tok.getLiteral() + "\n");
    }
};

class ArithExpression : public Expression
{
  protected:
    // Unique is also good but it will incur errors when
    // invoking copy constructors
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;

  public:

    // unique can be easily converted to shared
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

    // Debug print
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
};

/* Statement definition*/
class Statement
{
  public:
    enum class StatementType : int
    {
        SET_STATEMENT,
        FUNC_STATEMENT,
        RET_STATEMENT,
        ILLEGAL
    };

  protected:
    StatementType type = StatementType::ILLEGAL;

  public:
    Statement() {}

    virtual void printStatement() {}
};

class RetStatement : public Statement
{
  protected:
    std::string ret;

  public:
    RetStatement(std::string &_ret) : ret(_ret)
    {
        type = StatementType::RET_STATEMENT;
    }

    RetStatement(const RetStatement &_statement)
    {
        type = _statement.type;
        ret = _statement.ret;
    }
    
    void printStatement() override;
};

class SetStatement : public Statement
{
  protected:
    // using shared due to copy constructors
    std::shared_ptr<Identifier> iden;
    std::shared_ptr<Expression> expr;

  public:
    SetStatement(std::unique_ptr<Identifier> &_iden,
                 std::unique_ptr<Expression> &_expr)
    {
        type = StatementType::SET_STATEMENT;

        iden = std::move(_iden);
        expr = std::move(_expr);
    }
    
    SetStatement(const SetStatement &_statement)
    {
        iden = std::move(_statement.iden);
        expr = std::move(_statement.expr);
        type = _statement.type;
    }

    void printStatement() override;
};

class FuncStatement : public Statement
{
  public:
    enum class FuncType : int
    {
        // So far, we only support function type to be
        // void, int, or float
        VOID, INT, FLOAT, MAX
    };

    class Argument
    {
      public:
        enum class ArgumentType : int
        {
            // So far, we only support argument type to be
            // integer or float
            INT, FLOAT, MAX
        };

      protected:
        ArgumentType type = ArgumentType::MAX;
        Token tok;

      public:
        Argument(std::string &_type, Token &_tok)
        {
            if (_type == "int") type = ArgumentType::INT;
            else if (_type == "float") type = ArgumentType::FLOAT;
            else type = ArgumentType::MAX;

            assert(type != ArgumentType::MAX);

            tok = _tok;
        }

        std::string print()
        {
            std::string ret = "";
            if (type == ArgumentType::INT) ret += "int : ";
            else if (type == ArgumentType::FLOAT) ret += "float : ";

            ret += tok.getLiteral();

            return ret;
        }
    };

  protected:
    // using shared due to copy constructors
    FuncType func_type;
    std::shared_ptr<Identifier> iden;
    std::vector<Argument> args;
    std::vector<std::shared_ptr<Statement>> codes;

  // protected:
    // Track each local variable's type
    // std::unordered_map<std::string,Token> local_var_type_tracker;

  public:
    FuncStatement(FuncType _type,
                  std::unique_ptr<Identifier> &_iden,
                  std::vector<Argument> &_args,
                  std::vector<std::shared_ptr<Statement>> &_codes)
    {
        type = StatementType::FUNC_STATEMENT;

        func_type = _type;
        iden = std::move(_iden);
        args = _args;
        codes = std::move(_codes);
    }
    
    FuncStatement(const FuncStatement &_statement)
    {
        type = _statement.type;

        func_type = _statement.func_type;
        iden = std::move(_statement.iden);
        args = _statement.args;
        codes = std::move(_statement.codes);
    }
    
    void printStatement() override;

    // void recordLocalVarType(std::unordered_map<std::string,Token> &_tracker)
    // {
    //     local_var_type_tracker = _tracker;
    // }
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
  // TODO, should we put here?
  protected:
    static std::unique_ptr<LLVMContext> TheContext;
    static std::unique_ptr<Module> TheModule;
    static std::unique_ptr<IRBuilder<>> Builder;

  protected:
    Program program;

  protected:
    Token cur_token;
    Token next_token;    

  protected:
    // We force all the elements inside an expression with the
    // same type.
    Token::TokenType cur_expr_type;

    // Track each local variable's type
    std::unordered_map<std::string,Token::TokenType> local_var_type_tracker;

    void strictTypeCheck(Token &_tok)
    {
        auto check = _tok.getTokenType();

        if (auto t_iter = local_var_type_tracker.find(_tok.getLiteral());
                t_iter != local_var_type_tracker.end())
        {
            check = t_iter->second;
        }

        assert(check == cur_expr_type && 
               "[Error] Strict type check failed or undefined variable");
    }

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
