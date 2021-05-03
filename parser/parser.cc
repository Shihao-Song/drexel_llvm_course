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
    advanceTokens();
    // (1) parse identifier
    std::unique_ptr<Identifier> iden(new Identifier(cur_token));
    // std::cout << iden->getLiteral() << "\n";

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

    if (cur_token.isTokenInt() || cur_token.isTokenFloat())
    {
        if (next_token.isTokenSemicolon())
        {
            expression = 
                std::make_unique<LiteralExpression>(LiteralExpression(cur_token));
        }
    }
    return expression;
}

void SetStatement::printStatement()
{
    std::cout << "{\n";
    std::cout << "  type: set-statement \n";
    std::cout << "  left: " << iden->print() << "\n";
    std::cout << "  right: "<< "\n";
    std::cout << expr->print(1) << "\n";
    std::cout << "}\n\n";
}

std::string LiteralExpression::print(unsigned level)
{
    std::string pre_brace(level * 2, ' ');
    std::string pre_normal(level * 4, ' ');
    std::string ret = pre_brace + "{\n";
    ret += (pre_normal + "type: literal-expression\n");
    ret += (pre_normal + "literal: ");
    ret += ("{ type: " + tok.prinTokenType() + ", ");
    ret += ("value: " + tok.getLiteral() + " } \n");
    ret += (pre_brace + "}");
    return ret;
}

}
