#include "parser/parser.hh"

#include <cassert>

namespace Frontend
{
Parser::Parser(const char* fn) : lexer(new Lexer(fn))
{
    lexer->getToken(cur_token);
    lexer->getToken(next_token);

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
        FuncStatement::FuncType func_type;
        std::unique_ptr<Identifier> iden;
        std::vector<FuncStatement::Argument> args;
        std::vector<std::shared_ptr<Statement>> codes;

        if (cur_token.isTokenMain())
        {
            // We assume main() always return null
            func_type = FuncStatement::FuncType::VOID;

            iden = std::make_unique<Identifier>(cur_token);

            // We assume main() has no arguments
            advanceTokens();
            assert(cur_token.isTokenLP());

            advanceTokens();
            assert(cur_token.isTokenRP());

            advanceTokens();
            assert(cur_token.isTokenLB());
        }
        else if (cur_token.isTokenDef())
        {
            advanceTokens();
            assert(cur_token.isTokenLT());

            advanceTokens();
            if (cur_token.isTokenDesVoid())
            {
                func_type = FuncStatement::FuncType::VOID;
            }
            else if (cur_token.isTokenDesInt())
            {
                func_type = FuncStatement::FuncType::INT;
            }
	    else if (cur_token.isTokenDesFloat())
            {
                func_type = FuncStatement::FuncType::FLOAT;
            }
            else
            {
                std::cerr << "[Error] parseProgram: unsupported return type\n";
                exit(0);
            }
            
            advanceTokens();
            assert(cur_token.isTokenGT());

            advanceTokens();
            iden = std::make_unique<Identifier>(cur_token);
            if (auto f_iter = func_tracker.find(cur_token.getLiteral());
                      f_iter == func_tracker.end())
            {
                std::vector<Token::TokenType> init;
                func_tracker.insert({cur_token.getLiteral(), init});
            }
            else
            {
                std::cerr << "[Error] parseProgram: "
                          << "duplicated function definition. "
                          << std::endl;

                exit(0);
            }

            advanceTokens();
            assert(cur_token.isTokenLP());
            while (!cur_token.isTokenRP())
            {
                Token::TokenType type_track;

                advanceTokens();
                std::string arg_type = cur_token.getLiteral();
                if (arg_type == "int")
                {
                    type_track = Token::TokenType::TOKEN_INT;
                }
                else if (arg_type == "float")
                {
                    type_track = Token::TokenType::TOKEN_FLOAT;
                }
                else
                {
                    std::cerr << "[Error] parseProgram: "
                              << "unsupported variable type."
                              << std::endl;
                    exit(0);
                }

                advanceTokens();
                if (auto t_iter = local_var_type_tracker.
                                  find(cur_token.getLiteral());
                        t_iter != local_var_type_tracker.end())
                {
                    std::cout << "[Error] parseProgram: "
                              << "duplicated variable definition."
                              << std::endl;
                    exit(0);
                }
                else
                {
                    local_var_type_tracker.insert(
                        {cur_token.getLiteral(), type_track});
                }
                args.emplace_back(arg_type, cur_token);


                advanceTokens();
            }
            assert(cur_token.isTokenRP());

            advanceTokens();
            assert(cur_token.isTokenLB());
        }

        while (!cur_token.isTokenRB())
        {
            advanceTokens();
            if (cur_token.isTokenSet())
            {
                auto code = parseSetStatement();
                // code->printStatement();
                codes.push_back(std::move(code));
            }
            else if (cur_token.isTokenReturn())
            {
                advanceTokens();

                std::unique_ptr<RetStatement> ret_statement = 
                    std::make_unique<RetStatement>(cur_token.getLiteral());

                // check return type
                Token::TokenType check = cur_token.getTokenType();
                if (auto t_iter = local_var_type_tracker.find(
                                      cur_token.getLiteral());
                        t_iter != local_var_type_tracker.end())
                {
                    check = t_iter->second;
                }
                
                if (check == Token::TokenType::TOKEN_INT)
                {
                    assert(func_type == FuncStatement::FuncType::INT && 
                           "[parseProgram] Unmatched Return Type\n");
                }
                else if (check == Token::TokenType::TOKEN_FLOAT)
                {
                    assert(func_type == FuncStatement::FuncType::FLOAT && 
                           "[parseProgram] Unmatched Return Type\n");
                }
                else
                {
                    std::cerr << "[parseProgram] Unmatched Return Type\n";
                    exit(0);
                }

                codes.push_back(std::move(ret_statement));
            }
        }

        std::unique_ptr<Statement> func_proto
            (new FuncStatement(func_type, 
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

    advanceTokens();
    Token::TokenType var_type;
    if (cur_token.isTokenDesInt())
    {
        var_type = Token::TokenType::TOKEN_INT;
    }
    else if (cur_token.isTokenDesFloat())
    {
        var_type = Token::TokenType::TOKEN_FLOAT;
    }
    else
    {
        std::cerr << "[Error] parseSetStatement: "
                  << "unsupported variable type."
                  << std::endl;
        exit(0);
    }

    advanceTokens();
    assert(cur_token.isTokenGT());

    // (1) parse identifier
    advanceTokens();
    std::unique_ptr<Identifier> iden(new Identifier(cur_token));

    // (2) record variable type
    cur_expr_type = var_type;

    if (auto t_iter = local_var_type_tracker.find(cur_token.getLiteral());
            t_iter != local_var_type_tracker.end())
    {
        t_iter->second = var_type;
    }
    else
    {
        local_var_type_tracker.insert({cur_token.getLiteral(), var_type});
    }

    advanceTokens();
    assert(cur_token.isTokenEqual());

    // (3) parse expression
    advanceTokens();
    auto expr = parseExpression();

    // (4) prepare final statement
    std::unique_ptr<Statement> statement = 
        std::make_unique<SetStatement>(iden, expr);

    return statement;
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

            // We are trying to add/minus something with higher priority
            if (cur_token.isTokenLP() || 
                next_token.isTokenAsterisk() || 
                next_token.isTokenSlash())
            {
                right = parseTerm();
            }
            else
            {
                strictTypeCheck(cur_token);
                right = std::make_unique<LiteralExpression>(cur_token);
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
std::unique_ptr<Expression> Parser::parseTerm()
{   
    std::unique_ptr<Expression> left = parseFactor();

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
    left = std::make_unique<LiteralExpression>(cur_token);
 
    advanceTokens();

    return left;
}

void RetStatement::printStatement()
{
    std::cout << "    {\n";
    std::cout << "      Return\n";
    std::cout << "      " << ret << "\n";
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
    if (func_type == FuncType::VOID)
    {
        std::cout << "void\n";
    }
    else if (func_type == FuncType::INT)
    {
        std::cout << "int\n";
    }
    else if (func_type == FuncType::FLOAT)
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
