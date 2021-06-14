#include "parser/parser.hh"

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
        std::unique_ptr<Statement> statement = nullptr;
        if (cur_token.isTokenSet())
	{
            statement = parseSetStatement();
        }

        if (statement != nullptr)
        {
            program.addStatement(statement);
        }
        advanceTokens();
    }
}

std::unique_ptr<Statement> Parser::parseSetStatement()
{
    advanceTokens();
    // (1) parse identifier
    std::unique_ptr<Identifier> iden(new Identifier(cur_token));

    // (2) check error
    advanceTokens();
    if (!cur_token.isTokenEqual())
    {
        std::cerr << "[Error parseSetStatement] No equal sign. "
                  << std::endl;
        exit(0);
    }

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
            if (next_token.isTokenAsterisk() || 
                next_token.isTokenSlash())
            {
                right = parseTerm();
            }
            else
            {
                right = std::make_unique<LiteralExpression>(cur_token);
                advanceTokens();
            }

            left = std::make_unique<ArithExpression>(left, 
                       right, 
                       expr_type);
        }
        else if (cur_token.isTokenSemicolon())
        {
            return left;
        }
    }
}

// For Div/Mul
std::unique_ptr<Expression> Parser::parseTerm()
{   
    std::unique_ptr<Expression> left = 
        std::make_unique<LiteralExpression>(cur_token);
    
    advanceTokens();

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

            std::unique_ptr<Expression> right = 
                std::make_unique<LiteralExpression>(cur_token);
            
            left = std::make_unique<ArithExpression>(left, 
                       right, 
                       expr_type);

            advanceTokens();
        }
        else
        {
            break;
        }

    }

    return left;
}

std::unique_ptr<Expression> Parser::parseFactor()
{

}

void SetStatement::printStatement()
{
    std::cout << "{\n";
    std::cout << "Tree-stype: \n";
    std::cout << "  " + iden->print() << "\n";
    std::cout << "  =\n";
    if (expr->getType() == Expression::ExpressionType::LITERAL)
    {
        std::cout << "  " << expr->print(2);
    }
    else
    {
        std::cout << expr->print(2);
    }

    std::cout << "\nExpr: \n";
    std::cout << iden->print() << " = ";
    std::cout << expr->printExpr() << "\n";
    std::cout << "}\n";
}
}