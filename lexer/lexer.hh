#ifndef __LEXER_HH__
#define __LEXER_HH__

#include <fstream>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>

namespace Frontend
{
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
        TOKEN_MAIN, // main entrance of the program
        TOKEN_DEF,
        TOKEN_RETURN,
        TOKEN_SET,
		
        // For function/argument type
        // DES - description
        TOKEN_DES_VOID,
        TOKEN_DES_INT,
        TOKEN_DES_FLOAT,

        // Built-in functions
        TOKEN_PRINT_VAR_INT,
        TOKEN_PRINT_VAR_FLOAT
    } type = TokenType::TOKEN_ILLEGAL;

    std::string literal = "";

    Token() {}

    Token(const Token &_tok)
        : type(_tok.type)
        , literal(_tok.literal)
    {
    
    }

    Token(TokenType _type)
        : type(_type)
    {
    
    }

    Token(TokenType _type, std::string &_val)
        : type(_type)
        , literal(_val)
    {
    
    }

    std::string prinTokenType();

    auto &getLiteral() { return literal; }
    auto &getTokenType() { return type; }

    bool isTokenIden() { return type == TokenType::TOKEN_IDENTIFIER; }

    bool isTokenEOF() { return type == TokenType::TOKEN_EOF; }
    bool isTokenMain() { return type == TokenType::TOKEN_MAIN; }
    bool isTokenDef() { return type == TokenType::TOKEN_DEF; }
    bool isTokenReturn() { return type == TokenType::TOKEN_RETURN; }
    bool isTokenSet() { return type == TokenType::TOKEN_SET; }
    bool isTokenDesVoid() { return type == TokenType::TOKEN_DES_VOID; }
    bool isTokenDesInt() { return type == TokenType::TOKEN_DES_INT; }
    bool isTokenDesFloat() { return type == TokenType::TOKEN_DES_FLOAT; }

    bool isTokenInt() { return type == TokenType::TOKEN_INT; }
    bool isTokenFloat() { return type == TokenType::TOKEN_FLOAT; }
    bool isTokenPlus() { return type == TokenType::TOKEN_PLUS; }
    bool isTokenMinus() { return type == TokenType::TOKEN_MINUS; }
    bool isTokenAsterisk() { return type == TokenType::TOKEN_ASTERISK; }
    bool isTokenSlash() { return type == TokenType::TOKEN_SLASH; }
    bool isTokenArithOpr() 
    {
        return (isTokenPlus() || isTokenMinus() || 
                isTokenAsterisk() || isTokenSlash());
    }
    bool isTokenEqual() { return type == TokenType::TOKEN_ASSIGN; }

    bool isTokenComma() { return type == TokenType::TOKEN_COMMA; }
    bool isTokenSemicolon() { return type == TokenType::TOKEN_SEMICOLON; }
    bool isTokenLP() { return type == TokenType::TOKEN_LPAREN; }
    bool isTokenRP() { return type == TokenType::TOKEN_RPAREN; }
    bool isTokenLB() { return type == TokenType::TOKEN_LBRACE; }
    bool isTokenRB() { return type == TokenType::TOKEN_RBRACE; }

    bool isTokenLT() { return type == TokenType::TOKEN_LT; }
    bool isTokenGT() { return type == TokenType::TOKEN_GT; }

    bool isTokenPrintVarInt()
    {
        return type == TokenType::TOKEN_PRINT_VAR_INT;
    }
    bool isTokenPrintVarFloat()
    {
        return type == TokenType::TOKEN_PRINT_VAR_FLOAT;
    }
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
    ~Lexer() { code.close(); };

    bool getToken(Token&);

  protected:
    void parseLine(std::string &line);

    // helper function
    template<typename T>
    bool isType(std::string &cur_token_str)
    {
        std::istringstream iss(cur_token_str);
        T float_check;
        iss >> std::noskipws >> float_check;
        if (iss.eof() && !iss.fail()) return true;
        else return false;
    }

    bool isArithOpr(char c);
};

}

#endif
