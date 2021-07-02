#ifndef __PARSER_HH__
#define __PARSER_HH__

#include "lexer/lexer.hh"

#include <cassert>
#include <iostream>
#include <memory>
#include <variant>

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
        ARRAY,

        PLUS, // i.e., 1 + 2
        MINUS, // i.e., 1 - 2
        ASTERISK, // i.e., 1 * 2
        SLASH, // i.e., 1 / 2

        CALL,

        ILLEGAL
    };

  protected:

    ExpressionType type = ExpressionType::ILLEGAL;
    
  public:
    Expression() {}

    auto getType() { return type; }

    virtual std::string print(unsigned level) { return "[Error] No implementation"; }

    bool isExprLiteral() { return type == ExpressionType::LITERAL; }
    bool isExprArray() { return type == ExpressionType::ARRAY; }
    bool isExprCall() { return type == ExpressionType::CALL; }
    bool isExprArith()
    {
        return (type == ExpressionType::PLUS || 
                type == ExpressionType::MINUS ||
                type == ExpressionType::ASTERISK ||
                type == ExpressionType::SLASH);
    }
};

class CallExpression : public Expression
{
  protected:
    Token def;
    std::vector<std::shared_ptr<Expression>> args;

  public:
    CallExpression(const CallExpression &_expr) 
    {
        def = _expr.def;
        args = _expr.args;
        type = ExpressionType::CALL;
    }

    CallExpression(Token &_tok, 
        std::vector<std::shared_ptr<Expression>>_args) 
        : def(_tok)
        , args(_args)
    {
        type = ExpressionType::CALL;
    }

    // Debug print associated with the print in ArithExp
    std::string print(unsigned level) override
    {
        std::string prefix(level * 2, ' ');

        std::string ret = prefix + "{\n";
        ret += (prefix + "  [CALL] " + def.getLiteral() + "\n");
        unsigned idx = 0;
        for (auto &arg : args)
        {
            ret += (prefix + "  [ARG " + std::to_string(idx++) + "]\n");
            ret += (prefix + "  {\n");
            if (arg->getType() == Expression::ExpressionType::LITERAL)
                ret += (prefix + "    ");
            ret += arg->print(level + 2);
            ret += (prefix + "  }\n");
        }

	ret += (prefix + "}\n");
        return ret;
    }

    auto &getCallFunc() { return def.getLiteral(); }
    auto &getArgs() { return args; }
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

    std::string& getLiteral() { return tok.getLiteral(); }

    bool isLiteralInt() { return tok.isTokenInt(); }
    bool isLiteralFloat() { return tok.isTokenFloat(); }

    // Debug print associated with the print in ArithExp
    std::string print(unsigned level) override
    {
        return (tok.getLiteral() + "\n");
    }
};

class ArrayExpression : public Expression
{
  protected:
    std::shared_ptr<Expression> num_ele;
    std::vector<std::shared_ptr<Expression>> eles;

  public:
    ArrayExpression(std::unique_ptr<Expression> &_num_ele,
                    std::vector<std::shared_ptr<Expression>> &_eles)
    {
        num_ele = std::move(_num_ele);
        eles = std::move(_eles);
        type = ExpressionType::ARRAY;
    }

    ArrayExpression(const ArrayExpression &_expr)
    {
        num_ele = std::move(_expr.num_ele);
        eles = std::move(_expr.eles);
        type = _expr.type;
    }
    
    std::string print(unsigned level) override
    {
        std::string prefix(level * 2, ' ');

        std::string ret = prefix + "{\n";
        ret += (prefix + "  [ARRAY] \n");
        ret += (prefix + "  [NUM ELEMENTS]\n");
        ret += (prefix + "  {\n");
        if (num_ele->isExprLiteral())
            ret += (prefix + "    ");
        ret += num_ele->print(level + 2);
        ret += (prefix + "  }\n");

        ret += (prefix + "  [ELEMENTS]\n");
        ret += (prefix + "  {\n");
        for (auto &ele : eles)
        {
            ret += (prefix + "    {\n");
            if (ele->isExprLiteral())
                ret += (prefix + "      ");
            ret += ele->print(level + 3);
            ret += (prefix + "    }\n");
        }
        ret += (prefix + "  }\n");
	ret += (prefix + "}\n");
        return ret;
    }

};

// TODO-Shihao IndexExpression

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

    auto getLeft() { return left.get(); }
    auto getRight() { return right.get(); }

    char getOperator()
    {
        switch(type)
        {
            case ExpressionType::PLUS:
                return '+';
            case ExpressionType::MINUS:
                return '-';
            case ExpressionType::ASTERISK:
                return '*';
            case ExpressionType::SLASH:
                return '/';
            default:
                assert(false && "unsupported operator");
        }
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

            if (left->getType() == ExpressionType::CALL)
                ret += left->print(level);
            else
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

            if (right->getType() == ExpressionType::CALL)
                ret += right->print(level);
            else
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
        BUILT_IN_CALL_STATEMENT,
        NORMAL_CALL_STATEMENT,
        ILLEGAL
    };

  protected:
    StatementType type = StatementType::ILLEGAL;

  public:
    Statement() {}

    virtual void printStatement() {}

    bool isStatementFunc() { return type == StatementType::FUNC_STATEMENT; }
    bool isStatementSet() { return type == StatementType::SET_STATEMENT; }
    bool isStatementRet() { return type == StatementType::RET_STATEMENT; }
    bool isStatementBuiltinCall() 
    {
        return type == StatementType::BUILT_IN_CALL_STATEMENT; 
    }
    bool isStatementNormalCall()
    {
        return type == StatementType::NORMAL_CALL_STATEMENT;
    }
};

class CallStatement : public Statement
{
  protected:
    std::shared_ptr<Expression> expr;

  public:
    CallStatement(std::unique_ptr<Expression> &_expr,
                  StatementType _type)
    {
        type = _type;

        expr = std::move(_expr);
    }
    
    CallStatement(const CallStatement &_statement)
    {
        type = _statement.type;

        expr = std::move(_statement.expr);
    }
    
    void printStatement() override
    {
        std::cout << expr->print(2);
    }

    CallExpression* getCallExpr()
    {
        CallExpression *call = 
            static_cast<CallExpression*>(expr.get());
        return call;
    }
};

class RetStatement : public Statement
{
  protected:
    std::shared_ptr<Expression> ret;

  public:
    RetStatement(std::unique_ptr<Expression> &_ret) : ret(std::move(_ret))
    {
        type = StatementType::RET_STATEMENT;
    }

    RetStatement(const RetStatement &_statement)
    {
        type = _statement.type;
        ret = std::move(_statement.ret);
    }

    auto &getRetVal() { return ret; }

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

    auto &getIden() { return iden->getLiteral(); }
    auto getExpr() { return expr.get(); }

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
   
    bool isRetTypeVoid() { return func_type == RetType::VOID; }
    bool isRetTypeInt() { return func_type == RetType::INT; }
    bool isRetTypeFloat() { return func_type == RetType::FLOAT; }

    auto &getFuncName() { return iden->getLiteral(); }
    auto &getFuncArgs() { return args; }
    auto &getFuncCodes() { return codes; }

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

    auto& getStatements() { return statements; }
};

/* Parser definition */
class Parser
{
  protected:
    Program program;

  protected:
    Token cur_token;
    Token next_token;    
    
  public:
    /************* Section one - record local variable types ***************/
    enum class TypeRecord : int
    {
        VOID,
        VOID_PTR,

        INT,
        INT_ARRAY,
        INT_PTR,

        FLOAT,
        FLOAT_ARRAY,
        FLOAT_PTR,

        MAX
    };

  protected:
    // Track each local variable's type
    std::unordered_map<std::string,TypeRecord> local_var_type_tracker;

    // recordLocalVars v1 - record the arguments
    void recordLocalVars(FuncStatement::Argument &arg,
                         bool is_array = false,
                         bool is_ptr = false)
    {
        // determine argument type
        TypeRecord arg_type;
        if (arg.isArgInt())
        {
            if (is_array)
                arg_type = TypeRecord::INT_ARRAY;
            else
                arg_type = TypeRecord::INT;
        }
        else if (arg.isArgFloat())
        {
            if (is_array)
                arg_type = TypeRecord::FLOAT_ARRAY;
            else
                arg_type = TypeRecord::FLOAT;
        }
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
    void recordLocalVars(Token &_tok, Token &_type_tok,
                         bool is_array = false,
                         bool is_ptr = false)
    {
        // determine the token type
        TypeRecord var_type;
        if (_type_tok.isTokenDesInt())
        {
            cur_expr_type = TypeRecord::INT;
            if (is_array)
                var_type = TypeRecord::INT_ARRAY;
            else
                var_type = TypeRecord::INT;
        }
        else if (_type_tok.isTokenDesFloat())
        {
            cur_expr_type = TypeRecord::FLOAT;
            if (is_array)
                var_type = TypeRecord::FLOAT_ARRAY;
            else
                var_type = TypeRecord::FLOAT;
        }
        else
        {
            std::cerr << "[Error] recordLocalVars: "
                      << "unsupported variable type."
                      << std::endl;
            exit(0);
        }

	if (auto iter = local_var_type_tracker.find(_tok.getLiteral());
                iter != local_var_type_tracker.end())
        {
            assert(var_type == iter->second);
        }
        else
        {
            local_var_type_tracker[_tok.getLiteral()] = var_type;
        }
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
        if (_type == FuncStatement::RetType::VOID)
            ret_type = TypeRecord::VOID;
	else if (_type == FuncStatement::RetType::INT)
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
    std::unique_ptr<Lexer> lexer;

  public:
    Parser(const char* fn); 

    void printStatements() { program.printStatements(); }

    auto &getProgram() { return program; }

  protected:
    void parseProgram();
    void advanceTokens();

    std::unique_ptr<Statement> parseSetStatement();

    std::unique_ptr<Expression> parseArrayExpr();
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseTerm(
        std::unique_ptr<Expression> pending_left = nullptr);
    std::unique_ptr<Expression> parseFactor();

    std::unique_ptr<Expression> parseCall();

  protected:
    struct PerFuncRecord
    {
        PerFuncRecord() {}

        PerFuncRecord(std::unordered_map<std::string,TypeRecord> &_map)
        {
            var_to_type = _map;
        }

        void print()
        {
            for (auto &[var, type] : var_to_type)
            {
                std::cout << "  " << var << "\n";;
            }
        }

        TypeRecord getVarType(std::string &var_name)
        {
            auto iter = var_to_type.find(var_name);
            assert(iter != var_to_type.end());
            return iter->second;
        }

        std::unordered_map<std::string,TypeRecord> var_to_type;
    };

    std::unordered_map<std::string,PerFuncRecord> per_func_var_tracking;

    TypeRecord getInFuncVarType(std::string &func_name, 
                                std::string &var_name)
    {
        auto iter = per_func_var_tracking.find(func_name);
        assert(iter != per_func_var_tracking.end());

        return iter->second.getVarType(var_name);
    }

  public:
    bool isInFuncVarInt(std::string &func_name, std::string &var_name)
    {
        return getInFuncVarType(func_name, var_name) 
                   == TypeRecord::INT;
    }
    
    bool isInFuncVarFloat(std::string &func_name, std::string &var_name)
    {
         return getInFuncVarType(func_name, var_name) 
                   == TypeRecord::FLOAT;
    }

    auto& getFuncArgTypes(std::string &func_name)
    {
        auto iter = func_def_tracker.find(func_name);
        assert(iter != func_def_tracker.end());
        return iter->second.arg_types;
    }

    auto &getFuncRetType(std::string &_def)
    {
        auto iter = func_def_tracker.find(_def);
        assert(iter != func_def_tracker.end());

        return iter->second.ret_type;
    }

};
}

#endif
