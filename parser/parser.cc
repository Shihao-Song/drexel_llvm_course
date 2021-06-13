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
    // std::cout << "SET: " << iden->getLiteral() << "\n";

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
    std::unique_ptr<Expression> expression;

    if (cur_token.isTokenFunc())
    {

    }
    else
    {
        // We first extract the literal expression
        std::unique_ptr<Expression> left = 
            std::make_unique<LiteralExpression>(cur_token);
        // std::cout << left->print(0) << "\n";
        advanceTokens();

        while (true)
        {
            if (cur_token.isTokenPlus())
            {
                advanceTokens();
                std::unique_ptr<Expression> right = 
                    std::make_unique<LiteralExpression>(cur_token);
                // std::cout << right->print(0) << "\n";
                left = std::make_unique<ArithExpression>(left, 
                           right, 
                           Expression::ExpressionType::PLUS);
                // std::cout << left->print(0) << "\n";
                advanceTokens();
            }
            else if (cur_token.isTokenMinus())
            {
                advanceTokens();
                std::unique_ptr<Expression> right = 
                    std::make_unique<LiteralExpression>(cur_token);
                // std::cout << right->print(0) << "\n";
                left = std::make_unique<ArithExpression>(left, 
                           right, 
                           Expression::ExpressionType::MINUS);
                // std::cout << left->print(0) << "\n";
		advanceTokens();
            }
            else if (cur_token.isTokenSemicolon())
            {
                return left;
            }
        }
    }

    return expression;
}

void SetStatement::printStatement()
{
    std::cout << "{\n";
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
    std::cout << "}\n";
}
}
