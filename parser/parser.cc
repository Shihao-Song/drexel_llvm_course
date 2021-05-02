#include "parser/parser.hh"

namespace Frontend
{
bool Parser::advanceTokens()
{
    bool cur_aval = lexer->getToken(cur_token);
    bool next_aval = lexer->getToken(next_token);

    return cur_aval;
}

void Parser::parseProgram()
{
    while (advanceTokens())
    {
        std::unique_ptr<Statement> statement = nullptr;
        if (cur_token.isTokenSet())
	{
            statement = std::make_unique<SetStatement>();
        }

        if (statement != nullptr)
        {
            program.addStatement(statement);
        }
    }
}
}
