#ifndef __LEXER_HH__
#define __LEXER_HH__

#include <fstream>
#include <queue>
#include <string>
#include <unordered_map>

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

    std::string literal = "";

    Token() {}

    Token(const Token &_tok)
        : type(_tok.type)
        , literal(_tok.literal)
    {
    
    }

    Token(TokenType _type, std::string &_val)
        : type(_type)
        , literal(_val)
    {
    
    }

    void setTokenType();

    std::string prinTokenType();

    auto &getLiteral() { return literal; }

};

class Lexer
{
  protected:
    // define seperators
    std::unordered_map<char, Token::TokenType> seps;
    // define keywords
    std::unordered_map<std::string, Token::TokenType> keywords;

  protected:
    std::ifstream code;

    std::queue<Token> toks_per_line;

  public:
    Lexer(const char*);

    bool getToken(Token&);

  protected:
    void parseLine(std::string &line);
};
}

#endif
