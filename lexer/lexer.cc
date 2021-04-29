#include "lexer.hh"

#include <iostream>

namespace Frontend
{
std::string Token::prinTokenType(TokenType &tp)
{
    switch(tp)
    {
        case TokenType::TOKEN_ILLEGAL:
            return std::string("ILLEGAL");
        case TokenType::TOKEN_EOF:
            return std::string("EOF");
        case TokenType::TOKEN_IDENTIFIER:
            return std::string("IDENTIFIER");
        case TokenType::TOKEN_INT:
            return std::string("INT");
        case TokenType::TOKEN_FLOAT:
            return std::string("FLOAT");
        case TokenType::TOKEN_ASSIGN:
            return std::string("=");
        case TokenType::TOKEN_PLUS:
            return std::string("+");
        case TokenType::TOKEN_MINUS:
            return std::string("-");
        case TokenType::TOKEN_ASTERISK:
            return std::string("*");
        case TokenType::TOKEN_SLASH:
            return std::string("/");
        case TokenType::TOKEN_LT:
            return std::string("<");
        case TokenType::TOKEN_GT:
            return std::string(">");
        case TokenType::TOKEN_COMMA:
            return std::string(",");
        case TokenType::TOKEN_SEMICOLON:
            return std::string(";");
        case TokenType::TOKEN_LPAREN:
            return std::string("(");
        case TokenType::TOKEN_RPAREN:
            return std::string(")");
        case TokenType::TOKEN_LBRACE:
            return std::string("{");
        case TokenType::TOKEN_RBRACE:
            return std::string("}");
        case TokenType::TOKEN_FUNCTION:
            return std::string("FUNCTION");
        case TokenType::TOKEN_SET:
            return std::string("SET");
        default:
            std::cerr << "[Error] prinTokenType: "
                      << "unsupported token type. \n";
            exit(0);
    }
}
}
