#include "parser/parser.hh"

#include <iostream>

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
    std::unique_ptr<Statement> statement = 
        std::make_unique<SetStatement>();

    advanceTokens();
    // (1) parse identifier
    Identifier iden(cur_token);
    std::cout << iden.getLiteral() << "\n";

    // (2) check error
    advanceTokens();
    if (!cur_token.isTokenEqual())
    {
        std::cerr << "[Error parseSetStatement] No equal sign. "
                  << std::endl;
        exit(0);
    }

    // parse expression
    advanceTokens();
    auto expression = parseExpression();

    // modify statement

    return statement;
}

Expression Parser::parseExpression()
{
    Expression expression;

    if (cur_token.isTokenInt() || cur_token.isTokenFloat())
    {
        if (next_token.isTokenSemicolon())
        {
            if (cur_token.isTokenInt())
            {
                expression.literal = std::stoi(cur_token.getLiteral());
            }
            else
            {
                expression.literal = std::stof(cur_token.getLiteral());
            }
        }
    }

    return expression;
}

}
