#include "lexer/lexer.hh"

#include <cassert>
#include <iostream>
#include <sstream>

namespace Frontend
{
std::string Token::prinTokenType()
{
    switch(type)
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
            return std::string("ASSIGN");
        case TokenType::TOKEN_PLUS:
            return std::string("PLUS");
        case TokenType::TOKEN_MINUS:
            return std::string("MINUS");
        case TokenType::TOKEN_ASTERISK:
            return std::string("ASTERISK");
        case TokenType::TOKEN_SLASH:
            return std::string("SLASH");
        case TokenType::TOKEN_LT:
            return std::string("LT");
        case TokenType::TOKEN_GT:
            return std::string("GT");
        case TokenType::TOKEN_COMMA:
            return std::string("COMMA");
        case TokenType::TOKEN_SEMICOLON:
            return std::string("SEMICOLON");
        case TokenType::TOKEN_LPAREN:
            return std::string("LPAREN");
        case TokenType::TOKEN_RPAREN:
            return std::string("RPAREN");
        case TokenType::TOKEN_LBRACE:
            return std::string("LBRACE");
        case TokenType::TOKEN_RBRACE:
            return std::string("RBRACE");
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

Lexer::Lexer(const char* fn)
{
    code.open(fn);
    assert(code.good());

    // fill pre-defined seperators
    seps.insert({'=', Token::TokenType::TOKEN_ASSIGN});
    seps.insert({'+', Token::TokenType::TOKEN_PLUS});
    seps.insert({'-', Token::TokenType::TOKEN_MINUS});
    seps.insert({'*', Token::TokenType::TOKEN_BANG});
    seps.insert({'/', Token::TokenType::TOKEN_SLASH});
    seps.insert({'<', Token::TokenType::TOKEN_LT});
    seps.insert({'>', Token::TokenType::TOKEN_GT});
    seps.insert({',', Token::TokenType::TOKEN_COMMA});
    seps.insert({';', Token::TokenType::TOKEN_SEMICOLON});
    seps.insert({'(', Token::TokenType::TOKEN_LPAREN});
    seps.insert({')', Token::TokenType::TOKEN_RPAREN});
    seps.insert({'{', Token::TokenType::TOKEN_LBRACE});
    seps.insert({'}', Token::TokenType::TOKEN_RBRACE});

    // fill pre-defined keywords
    keywords.insert({"set", Token::TokenType::TOKEN_SET});
    keywords.insert({"function", Token::TokenType::TOKEN_FUNCTION});
}

bool Lexer::getToken(Token &tok)
{
    // Check if toks_per_line is empty or not
    if (toks_per_line.size())
    {
        tok = toks_per_line.front();
        toks_per_line.pop();
        return true;
    }

    // Read a line
    std::string line;
    getline(code, line);

    // Return if EOF
    if (code.eof()) return false;

    // Parse the line
    parseLine(line);

    // Skip empty lines
    while (toks_per_line.size() == 0)
    {
        getline(code, line);
        if (code.eof()) return false;
        parseLine(line);
    }

    assert(toks_per_line.size() != 0);
    // std::cout << "\n" << line << "\n";
    tok = toks_per_line.front();
    toks_per_line.pop();
    return true;
}

void Lexer::parseLine(std::string &line)
{
    // Extract all the tokens from the current line
    for (auto iter = line.begin(); iter != line.end(); iter++)
    {
        // (1) skip the space/tab
        if (*iter == ' ' || *iter == '\t') continue;

        // start to process token
        std::string cur_token_str(1, *iter);

        // (2) is it a sep?
        if (auto sep_iter = seps.find(*iter); 
            sep_iter != seps.end())
        {
            std::string literal = cur_token_str;
            Token::TokenType type = sep_iter->second;
            Token _tok(type, literal);
            // std::cout << _tok.prinTokenType() << " | "
            //           << _tok.getVal() << "\n";
            toks_per_line.push(_tok);
            continue;
        }

        // (3) parse the token
        auto next = iter + 1;
        while (next != line.end())
        {
            auto next_sep_check = seps.find(*next);
            if ((*next == ' ') || (next_sep_check != seps.end()))
            {
                break;
            }
            cur_token_str.push_back(*next);
            next++;
            iter++;
        }

        // is the token int?
	{
            std::istringstream iss(cur_token_str);	
	    int int_check;
            iss >> std::noskipws >> int_check;
            if (iss.eof() && !iss.fail())
            {
                std::string literal = cur_token_str;
                Token::TokenType type = Token::TokenType::TOKEN_INT;
                Token _tok(type, literal);
                // std::cout << _tok.prinTokenType() << " | "
                //           << _tok.getVal() << "\n";
                toks_per_line.push(_tok);
                continue;
            }
        }

        // is the token float?
        {
            std::istringstream iss(cur_token_str);	
	    float float_check;
            iss >> std::noskipws >> float_check;
            if (iss.eof() && !iss.fail())
            {
                std::string literal = cur_token_str;
                Token::TokenType type = Token::TokenType::TOKEN_FLOAT;
                Token _tok(type, literal);
                // std::cout << _tok.prinTokenType() << " | "
                //           << _tok.getVal() << "\n";
                toks_per_line.push(_tok);
                continue;
            }
        }

        // is the token keywork?
        if (auto k_iter = keywords.find(cur_token_str);
            k_iter != keywords.end())
        {
            std::string literal = cur_token_str;
            Token::TokenType type = k_iter->second;
            Token _tok(type, literal);
            // std::cout << _tok.prinTokenType() << " | "
            //           << _tok.getVal() << "\n";
            toks_per_line.push(_tok);
        }
        else
        {
	    std::string literal = cur_token_str;
            Token::TokenType type = Token::TokenType::TOKEN_IDENTIFIER;
            Token _tok(type, literal);
            // std::cout << _tok.prinTokenType() << " | "
            //           << _tok.getVal() << "\n";
            toks_per_line.push(_tok);
        }
    }
}
}
