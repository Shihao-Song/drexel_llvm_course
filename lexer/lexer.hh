#ifndef __LEXER_HH__
#define __LEXER_HH__

#include <string>

namespace Frontend
{

/*
 * set x = 3;
 * set y = 4;
 * set add = function(x, y)
 * {
 *     x + y;
 * }
 *
 * set result = add(x, y);
 * */

struct Token
{
    enum class TokenType : int
    {
        TOKEN_ILLEGAL,
        TOKEN_EOF,

        // Identifiers + literals
        TOKEN_IDENTIFIER, // i.e., x, y, i, j, ...
        TOKEN_INT, // i.e., 1, 2, 3, 4, ...
        TOKEN_FLOAT, // i.e., 1.1, 1.2, ...

        // Operators
        TOKEN_ASSIGN,
        TOKEN_PLUS,
        TOKEN_MINUS,
        TOKEN_BANG,
        TOKEN_ASTERISK,
        TOKEN_SLASH,

        TOKEN_LT,
        TOKEN_GT,

        // Delimiters
        TOKEN_COMMA,
        TOKEN_SEMICOLON,

	TOKEN_LPAREN,
        TOKEN_RPAREN,
        TOKEN_LBRACE,
        TOKEN_RBRACE,

        // Keywords
        TOKEN_FUNCTION,
        TOKEN_SET
    } type = TokenType::TOKEN_ILLEGAL;

    std::string literal;

    // struct functions
    void setTokenType();

    std::string prinTokenType(TokenType &tp);
};

class Lexer
{

  public:
    Lexer(std::string &fn) {}

    int getToken();
};
}

#endif
