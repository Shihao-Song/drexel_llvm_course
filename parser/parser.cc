#include "parser/parser.hh"

namespace Frontend
{
Parser::Parser(const char* fn) : lexer(new Lexer(fn))
{
    // Pre-load all the tokens
    lexer->getToken(cur_token);
    lexer->getToken(next_token);

    // Fill the pre-built 
    std::vector<TypeRecord> arg_types;
    TypeRecord ret_type = TypeRecord::VOID;
    FuncRecord record;

    // printVarInt
    arg_types.push_back(TypeRecord::INT);
    record.ret_type = ret_type;
    record.arg_types = arg_types;
    func_def_tracker.insert({"printVarInt", record});

    // printVarFloat
    arg_types.clear();
    arg_types.push_back(TypeRecord::FLOAT);
    record.ret_type = ret_type;
    record.arg_types = arg_types;
    func_def_tracker.insert({"printVarFloat", record});

    parseProgram();
}

void Parser::advanceTokens()
{
    cur_token = next_token;
    lexer->getToken(next_token);
}

void Parser::parseProgram()
{
    while (!cur_token.isTokenEOF())
    {
        FuncStatement::RetType ret_type;
        std::unique_ptr<Identifier> iden;
        std::vector<FuncStatement::Argument> args;
        std::vector<std::shared_ptr<Statement>> codes;

        if (cur_token.isTokenMain())
        {
            // We assume main() always return int
            ret_type = FuncStatement::RetType::INT;

            iden = std::make_unique<Identifier>(cur_token);

            // We assume main() has no arguments
            advanceTokens();
            assert(cur_token.isTokenLP());

            advanceTokens();
            assert(cur_token.isTokenRP());

            advanceTokens();
            assert(cur_token.isTokenLBrace());
        }
        else if (cur_token.isTokenDef())
        {
            advanceTokens();
            assert(cur_token.isTokenLT());

            // determine return type
            advanceTokens();
            if (cur_token.isTokenDesVoid())
            {
                ret_type = FuncStatement::RetType::VOID;
            }
            else if (cur_token.isTokenDesInt())
            {
                ret_type = FuncStatement::RetType::INT;
            }
	    else if (cur_token.isTokenDesFloat())
            {
                ret_type = FuncStatement::RetType::FLOAT;
            }
            else
            {
                std::cerr << "[Error] parseProgram: unsupported return type\n";
                exit(0);
            }
            
            advanceTokens();
            assert(cur_token.isTokenGT());

            // function name
            advanceTokens();
            iden = std::make_unique<Identifier>(cur_token);

            advanceTokens();
            assert(cur_token.isTokenLP());

            // extract arguments
            while (!cur_token.isTokenRP())
            {
                advanceTokens();
                if (cur_token.isTokenRP()) break; // no args

                std::string arg_type = cur_token.getLiteral();

                advanceTokens();
                FuncStatement::Argument arg(arg_type, cur_token);
                args.push_back(arg);

                recordLocalVars(arg);

                advanceTokens();
            }
            assert(cur_token.isTokenRP());

            advanceTokens();
            assert(cur_token.isTokenLBrace());

            // record function def
            recordDefs(iden->getLiteral(), ret_type, args);
        }
        else
        {
            assert(false && "Don't support global var yet");
        }

        // parse the codes section
        while (!cur_token.isTokenRBrace())
        {
            advanceTokens();
            if (cur_token.isTokenSet())
            {
                auto code = parseSetStatement();
                // code->printStatement();
                codes.push_back(std::move(code));
            }
            else if (cur_token.isTokenPrintVarInt() || 
                     cur_token.isTokenPrintVarFloat())
            {
                auto code = parseCall();
                std::unique_ptr<CallStatement> built_in = 
                    std::make_unique<CallStatement>(code, 
                        Statement::StatementType::BUILT_IN_CALL_STATEMENT);
                // built_in->printStatement();
                codes.push_back(std::move(built_in));
            }
            else if (cur_token.isTokenReturn())
            {
                advanceTokens();

                if (iden->getLiteral() == "main")
                    cur_expr_type = TypeRecord::INT;
                else
                    cur_expr_type = getFuncRetType(iden->getLiteral());

                auto ret = parseExpression();

                std::unique_ptr<RetStatement> ret_statement = 
                    std::make_unique<RetStatement>(ret);

                codes.push_back(std::move(ret_statement));
            }
            else
            {
                // Should be a normal function call
                if (auto iter = func_def_tracker.find(cur_token.getLiteral());
                        iter == func_def_tracker.end())
                {
                    continue;
                }
                auto code = parseCall();
                std::unique_ptr<CallStatement> call = 
                    std::make_unique<CallStatement>(code, 
                        Statement::StatementType::NORMAL_CALL_STATEMENT);

                codes.push_back(std::move(call));
            }
        }

        per_func_var_tracking.insert({iden->getLiteral(),
                                      PerFuncRecord(local_var_type_tracker)});
        local_var_type_tracker.clear();

        std::unique_ptr<Statement> func_proto
            (new FuncStatement(ret_type, 
                               iden, 
                               args, 
                               codes));
        program.addStatement(func_proto);
        
        advanceTokens();
    }
}

std::unique_ptr<Statement> Parser::parseSetStatement()
{
    advanceTokens();
    assert(cur_token.isTokenLT());

    // Extract var type
    advanceTokens();
    Token type_token = cur_token;
    bool is_array = false;
    if (cur_token.isTokenArray())
    {
        is_array = true;

        advanceTokens();
        assert(cur_token.isTokenLT());

        advanceTokens();
        type_token = cur_token;

        advanceTokens();
        assert(cur_token.isTokenGT());
    }

    advanceTokens();
    assert(cur_token.isTokenGT());

    // (1) parse identifier
    advanceTokens();
    std::unique_ptr<Identifier> iden(new Identifier(cur_token));

    // (2) record variable type
    recordLocalVars(cur_token, type_token, is_array);

    // (3) parse expression
    std::unique_ptr<Expression> expr;
    if (!is_array)
    {
        advanceTokens();
        assert(cur_token.isTokenEqual());

        advanceTokens();
        expr = parseExpression();
    }
    else
    {
        expr = parseArrayExpr();
    }

    // (4) prepare final statement
    std::unique_ptr<Statement> statement = 
        std::make_unique<SetStatement>(iden, expr);

    return statement;
}

std::unique_ptr<Expression> Parser::parseArrayExpr()
{
    advanceTokens();
    assert(cur_token.isTokenLBracket());

    advanceTokens();
    // num_ele must be an integer
    auto swap = cur_expr_type;
    cur_expr_type = TypeRecord::INT;
    auto num_ele = parseExpression();
    cur_expr_type = swap;
    assert(cur_token.isTokenRBracket());

    advanceTokens();
    assert(cur_token.isTokenEqual());

    advanceTokens();
    assert(cur_token.isTokenLBrace());

    std::vector<std::shared_ptr<Expression>> eles;
    advanceTokens();
    assert(!cur_token.isTokenRBrace());

    while (!cur_token.isTokenRBrace())
    {
        eles.push_back(parseExpression());
        if (cur_token.isTokenComma())
            advanceTokens();
    }

    advanceTokens();

    std::unique_ptr<Expression> ret = 
        make_unique<ArrayExpression>(num_ele, eles);

    return ret;
}

std::unique_ptr<Expression> Parser::parseExpression()
{
    std::unique_ptr<Expression> left = parseTerm();

    while (true)
    {
        if (cur_token.isTokenPlus() || 
            cur_token.isTokenMinus())
        {
            Expression::ExpressionType expr_type;
            if (cur_token.isTokenPlus())
            {
                expr_type = Expression::ExpressionType::PLUS;
            }
            else
            {
                expr_type = Expression::ExpressionType::MINUS;
            }

            advanceTokens();

            std::unique_ptr<Expression> right;

            // Priority one. ()
            if (cur_token.isTokenLP())
            {
                right = parseTerm();
                left = std::make_unique<ArithExpression>(left, 
                       right, 
                       expr_type);
                continue;
            }

            // Priority two. *, /
            // check if the next token is a function call
	    std::unique_ptr<Expression> call_expr = nullptr;
            if (isFuncDef(cur_token.getLiteral()))
            {
                strictTypeCheck(cur_token);
                call_expr = parseCall();
	    }

            if (next_token.isTokenAsterisk() ||
                next_token.isTokenSlash())
            {
                if (call_expr != nullptr)
                {
                    advanceTokens();
                    right = parseTerm(std::move(call_expr));
                }
                else
                {
                    right = parseTerm();
                }
            }
            else
            {
                if (call_expr != nullptr)
                {
                    right = std::move(call_expr);
                }
                else
                {
                    strictTypeCheck(cur_token);
                    right = std::make_unique<LiteralExpression>(cur_token);
                }
                advanceTokens();
            }

            left = std::make_unique<ArithExpression>(left, 
                       right, 
                       expr_type);
        }
        else
        {
            return left;
        }
    }
}

// For Div/Mul
std::unique_ptr<Expression> Parser::parseTerm(
    std::unique_ptr<Expression> pending_left)
{   
    std::unique_ptr<Expression> left = 
        (pending_left != nullptr) ? std::move(pending_left) : parseFactor();

    while (true)
    {
        if (cur_token.isTokenAsterisk() || 
            cur_token.isTokenSlash())
        {
            Expression::ExpressionType expr_type;
            if (cur_token.isTokenAsterisk())
            {
                expr_type = Expression::ExpressionType::ASTERISK;
            }
            else
            {
                expr_type = Expression::ExpressionType::SLASH;
            }

            advanceTokens();

            std::unique_ptr<Expression> right;

            // We are trying to mul/div something with higher priority
            if (cur_token.isTokenLP()) 
            {
                right = parseTerm();
            }
            else
            {
                strictTypeCheck(cur_token);
                // check if it is a function call
                if (isFuncDef(cur_token.getLiteral()))
                    right = parseCall();
                else
                    right = std::make_unique<LiteralExpression>(cur_token);
                advanceTokens();
            }

            left = std::make_unique<ArithExpression>(left, 
                       right, 
                       expr_type);
        }
        else
        {
            break;
        }

    }

    return left;
}

// Deal with () here
std::unique_ptr<Expression> Parser::parseFactor()
{
    std::unique_ptr<Expression> left;

    if (cur_token.isTokenLP())
    {
        advanceTokens();
        left = parseExpression();
        assert(cur_token.isTokenRP()); // Error checking
        advanceTokens();
        return left;
    }

    strictTypeCheck(cur_token);
    if (isFuncDef(cur_token.getLiteral()))
        left = parseCall();
    else
        left = std::make_unique<LiteralExpression>(cur_token);
 
    advanceTokens();

    return left;
}

std::unique_ptr<Expression> Parser::parseCall()
{
    Token def_tok = cur_token;

    advanceTokens();
    assert(cur_token.isTokenLP());

    advanceTokens();
    std::vector<std::shared_ptr<Expression>> args;

    auto &arg_types = getFuncArgTypes(def_tok.getLiteral());
    unsigned idx = 0;
    while (!cur_token.isTokenRP())
    {
        if (cur_token.isTokenRP())
            break;

        auto swap = cur_expr_type;
        cur_expr_type = arg_types[idx++];
        args.push_back(parseExpression());
        cur_expr_type = swap;

        if (cur_token.isTokenRP())
            break;
        advanceTokens();
    }

    std::unique_ptr<CallExpression> ret = 
        std::make_unique<CallExpression>(def_tok, args);

    return ret;
}

void RetStatement::printStatement()
{
    std::cout << "    {\n";
    std::cout << "      [Return]\n";
    if (ret->getType() == Expression::ExpressionType::LITERAL)
    {
        std::cout << "      " << ret->print(4);
    }
    else
    {
        std::cout << ret->print(4);
    }
    std::cout << "    }\n";
}

void SetStatement::printStatement()
{
    std::cout << "    {\n";
    std::cout << "      " + iden->print() << "\n";
    std::cout << "      =\n";
    if (expr->getType() == Expression::ExpressionType::LITERAL)
    {
        std::cout << "      " << expr->print(4);
    }
    else
    {
        std::cout << expr->print(4);
    }

    std::cout << "    }\n";
}

void FuncStatement::printStatement()
{
    std::cout << "{\n";
    std::cout << "  Function Name: " << iden->print() << "\n";
    std::cout << "  Return Type: ";
    if (func_type == RetType::VOID)
    {
        std::cout << "void\n";
    }
    else if (func_type == RetType::INT)
    {
        std::cout << "int\n";
    }
    else if (func_type == RetType::FLOAT)
    {
        std::cout << "float\n";
    }

    std::cout << "  Arguments\n";
    for (auto &arg : args)
    {
        std::cout << "    " << arg.print() << "\n";
    }
    if (!args.size()) std::cout << "    NONE\n";

    std::cout << "  Codes\n";
    std::cout << "  {\n";
    for (auto &code : codes)
    {
        code->printStatement();
    }
    std::cout << "  }\n";
    std::cout << "}\n";
}
}
