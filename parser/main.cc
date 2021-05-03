#include "lexer/lexer.hh"
#include "parser/parser.hh"

#include <iomanip>
#include <iostream>

using namespace Frontend;

int main(int argc, char* argv[])
{
    /*
    // Lexer
    Lexer lexer(argv[1]);

    Token tok;
    while (lexer.getToken(tok))
    {
        
        std::cout << std::setw(12)
                  << tok.prinTokenType() << " | "
                  << tok.getLiteral() << "\n";
        
    }
    */

    // Parser
    Parser parser(argv[1]);
    parser.printStatements();
}
