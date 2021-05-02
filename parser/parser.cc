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
    for (advanceTokens())
    {
        StatementPtr statement = nullptr;

        
    }
}

}
