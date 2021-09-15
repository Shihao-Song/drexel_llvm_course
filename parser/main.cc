/* Parser driver code.
 * Author: Shihao Song
 * Modified by: Naga Kandasamy
 * Date: September 14, 2021
 */
#include "lexer/lexer.hh"
#include "parser/parser.hh"

#include <iomanip>
#include <iostream>

using namespace Frontend;

int main(int argc, char *argv[])
{
    Parser parser(argv[1]); // Instantiate Parser object
    parser.printStatements();
}
