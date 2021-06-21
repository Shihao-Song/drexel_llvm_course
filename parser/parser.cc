#include "parser/parser.hh"

namespace Frontend
{
Parser::Parser(const char* fn) : lexer(new Lexer(fn))
{
    // Pre-load all the tokens
    Token tok;
    while (lexer->getToken(tok))
    {
        tokens.push_back(tok);
    }
    tokens.push_back(tok);

    cur_token = tokens[token_index];
    next_token = tokens[token_index + 1];

    parseProgram();
}

void Parser::advanceTokens()
{
    token_index += 1;
    if (token_index == tokens.size()) return;

    cur_token = tokens[token_index];

    if ((token_index + 1) == tokens.size()) return;
    next_token = tokens[token_index + 1];
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
            // We assume main() always return null
            ret_type = FuncStatement::RetType::VOID;

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
                std::string arg_type = cur_token.getLiteral();

                advanceTokens();
                FuncStatement::Argument arg(arg_type, cur_token);
                args.push_back(arg);

                recordLocalVars(arg);

                advanceTokens();
            }
            assert(cur_token.isTokenRP());

            advanceTokens();
            assert(cur_token.isTokenLB());

            // record function def
            recordDefs(iden->getLiteral(), ret_type, args);
        }

        // parse the codes section
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

                strictTypeCheck(cur_token, ret_type);

                std::unique_ptr<RetStatement> ret_statement = 
                    std::make_unique<RetStatement>(cur_token.getLiteral());
                // ret_statement->printStatement();

                codes.push_back(std::move(ret_statement));
            }
        }

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

    advanceTokens();
    Token type_token = cur_token;

    advanceTokens();
    assert(cur_token.isTokenGT());

    // (1) parse identifier
    advanceTokens();
    std::unique_ptr<Identifier> iden(new Identifier(cur_token));

    // (2) record variable type
    recordLocalVars(cur_token, type_token);
    
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
                peakNextArithOpr().isTokenAsterisk() || 
                peakNextArithOpr().isTokenSlash())
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
    std::vector<Token> args;
    while (!cur_token.isTokenRP())
    {
        if (cur_token.isTokenComma()) advanceTokens();

        args.push_back(cur_token);
        advanceTokens();
    }

    strictTypeCheck(def_tok, args);

    std::unique_ptr<CallExpression> ret = 
        std::make_unique<CallExpression>(def_tok, args);

    // advanceTokens();

    return ret;
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
    if (expr->getType() == Expression::ExpressionType::LITERAL ||
        expr->getType() == Expression::ExpressionType::CALL)
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
