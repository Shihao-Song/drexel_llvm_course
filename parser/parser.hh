#ifndef __PARSER_HH__
#define __PARSER_HH__

#include "lexer/lexer.hh"

#include <cassert>
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

class CallExpression : public Expression
{
  protected:
    Token def;
    std::vector<Token> args;  

  public:
    CallExpression(const CallExpression &_expr) 
    {
        def = _expr.def;
        args = _expr.args;
        type = ExpressionType::CALL;
    }

    CallExpression(Token &_tok, std::vector<Token> _args) 
        : def(_tok)
        , args(_args)
    {
        type = ExpressionType::CALL;
    }

    // Debug print associated with the print in ArithExp
    std::string print(unsigned level) override
    {
        std::string ret = "[CALL] " + def.getLiteral()
                        + " [ARGS] ";
        for (auto &arg : args) ret += (arg.getLiteral() + " ");

	ret += "\n";
        return ret;
    }
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
            if (left->getType() == ExpressionType::LITERAL || 
                left->getType() == ExpressionType::CALL)
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
            
            if (right->getType() == ExpressionType::LITERAL || 
                right->getType() == ExpressionType::CALL)
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
    enum class RetType : int
    {
        // So far, we only support function return type to be
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

        std::string &getLiteral() { return tok.getLiteral(); }
        bool isArgInt() { return type == ArgumentType::INT; }
        bool isArgFloat() { return type == ArgumentType::FLOAT; }
    };

  protected:
    // using shared due to copy constructors
    RetType func_type;
    std::shared_ptr<Identifier> iden;
    std::vector<Argument> args;
    std::vector<std::shared_ptr<Statement>> codes;

  public:
    FuncStatement(RetType _type,
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
    uint32_t token_index = 0;
    Token cur_token;
    Token next_token;    

    // TODO-Future work, there may be an expression as the argument
    // of call expression. So, add(x,y) is supported, but something
    // like add(x, y + x) is not supported.
    Token peakNextArithOpr()
    {
        auto iter = token_index + 1;
        while (true)
        {
            if (iter == tokens.size()) 
                return Token(Token::TokenType::TOKEN_EOF);

            Token &cur_tok = tokens[iter];
            if (cur_tok.isTokenArithOpr()) return cur_tok;
            iter++;
        }
    }

  protected:
    /************* Section one - record local variable types ***************/
    enum class TypeRecord : int
    {
        INT, FLOAT, MAX
    };

    // Track each local variable's type
    std::unordered_map<std::string,TypeRecord> local_var_type_tracker;

    // recordLocalVars v1 - record the arguments
    void recordLocalVars(FuncStatement::Argument &arg)
    {
        // determine argument type
        TypeRecord arg_type = TypeRecord::MAX;
        if (arg.isArgInt()) arg_type = TypeRecord::INT;
        else if (arg.isArgFloat()) arg_type = TypeRecord::FLOAT;
        assert(arg_type != TypeRecord::MAX);

        // argument name can not be duplicated
        auto arg_name = arg.getLiteral();

        if (auto iter = local_var_type_tracker.find(arg_name);
                iter != local_var_type_tracker.end())
        {
            std::cerr << "[Error] recordLocalVars: "
                      << "duplicated variable definition."
                      << std::endl;
            exit(0);
        }
        else
        {
            local_var_type_tracker.insert({arg_name, arg_type});
        }
    }
    // recordLocalVars v2 - record local variables
    void recordLocalVars(Token &_tok, Token &_type_tok)
    {
        // determine the token type
        TypeRecord var_type;
        if (_type_tok.isTokenDesInt())
        {
            var_type = TypeRecord::INT;
        }
        else if (_type_tok.isTokenDesFloat())
        {
            var_type = TypeRecord::FLOAT;
        }
        else
        {
            std::cerr << "[Error] recordLocalVars: "
                      << "unsupported variable type."
                      << std::endl;
            exit(0);
        }
        cur_expr_type = var_type;

        local_var_type_tracker[_tok.getLiteral()] = var_type;
    }

    /************* Section two - record function informatoin ****************/
    struct FuncRecord
    {
        TypeRecord ret_type;
        std::vector<TypeRecord> arg_types;

        FuncRecord() {}
    };
    std::unordered_map<std::string,FuncRecord> func_def_tracker;
    void recordDefs(std::string &_def,
                    FuncStatement::RetType _type,
                    std::vector<FuncStatement::Argument> &_args)
    {
        auto iter = func_def_tracker.find(_def);
        assert(iter == func_def_tracker.end() && "duplicated def");

        FuncRecord record;
        auto &ret_type = record.ret_type;
        if (_type == FuncStatement::RetType::INT)
            ret_type = TypeRecord::INT;
        else if (_type == FuncStatement::RetType::FLOAT)
            ret_type = TypeRecord::FLOAT;
        else
            assert(false && "unsupported return type");

        auto &arg_types = record.arg_types;
        for (auto &arg : _args)
        {
            if (arg.isArgInt()) arg_types.push_back(TypeRecord::INT);
            else if (arg.isArgFloat()) arg_types.push_back(TypeRecord::FLOAT);
            else assert(false && "unsupported argument type");
        }
        
        func_def_tracker[_def] = record;
    }

    bool isFuncDef(std::string &_def)
    {
        if (auto iter = func_def_tracker.find(_def);
                iter != func_def_tracker.end())
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    /***************** Section two - strict type checking ******************/

    // We force all the elements inside an EXPRESSION with the
    // same type.
    TypeRecord cur_expr_type;
    // strictTypeCheck v1 - check the token has the same type as the
    // cur_expr_type.
    void strictTypeCheck(Token &_tok)
    {
        TypeRecord tok_type = getTokenType(_tok);
        assert(tok_type == cur_expr_type && 
               "[Error] Inconsistent type within expression \
               or undefined variable");
    }
    // strictTypeCheck v2 - check the token has the same type as the specified
    // function return type, usually for return statement
    void strictTypeCheck(Token &_tok, FuncStatement::RetType _type)
    {
        TypeRecord tok_type = getTokenType(_tok);

        if (tok_type == TypeRecord::INT && 
            _type == FuncStatement::RetType::INT) return;
	
        if (tok_type == TypeRecord::FLOAT && 
            _type == FuncStatement::RetType::FLOAT) return;

        assert(false && 
               "[Error] Inconsistent return type \
               or undefined variable");
    }
    // strictTypeCheck v3 - check if the passed tokens have the same type as
    // the function def
    void strictTypeCheck(Token &_def_tok, std::vector<Token> args)
    {
        auto iter = func_def_tracker.find(_def_tok.getLiteral());
        assert(iter != func_def_tracker.end());

        auto &ans = iter->second.arg_types;
        assert(ans.size() == args.size() &&
               "#params don't match");

        for (auto i = 0; i < args.size(); i++)
        {
            TypeRecord arg_type = getTokenType(args[i]);
            assert(arg_type == ans[i] && 
                   "param types don't match");
        }
    }

    TypeRecord getTokenType(Token &_tok)
    {
        TypeRecord tok_type;
        if (_tok.isTokenInt()) tok_type = TypeRecord::INT;
        else if (_tok.isTokenFloat()) tok_type = TypeRecord::FLOAT;
        else tok_type = TypeRecord::MAX;

        // If the token is a variable, we need extract its recorded type
        if (auto iter = local_var_type_tracker.find(_tok.getLiteral());
                iter != local_var_type_tracker.end())
        {
            tok_type = iter->second;
        }

        // If the token is function name, we need to extract its
        // recorded type.
        if (auto iter = func_def_tracker.find(_tok.getLiteral());
                iter != func_def_tracker.end())
        {
            tok_type = iter->second.ret_type;
        }
        
        return tok_type;
    }

  protected:
    std::vector<Token> tokens;
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
    std::unique_ptr<Expression> parseCall();
};
}

#endif
