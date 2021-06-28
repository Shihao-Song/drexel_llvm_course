#include "lexer/lexer.hh"

#include <cassert>
#include <iostream>

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
        case TokenType::TOKEN_BANG:
            return std::string("BANG");
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
        case TokenType::TOKEN_MAIN:
            return std::string("MAIN");
        case TokenType::TOKEN_DEF:
            return std::string("DEF");
        case TokenType::TOKEN_RETURN:
            return std::string("RETURN");
        case TokenType::TOKEN_SET:
            return std::string("SET");
        case TokenType::TOKEN_DES_VOID:
            return std::string("DES-VOID");
        case TokenType::TOKEN_DES_INT:
            return std::string("DES-INT");
        case TokenType::TOKEN_DES_FLOAT:
            return std::string("DES-FLOAT");
        case TokenType::TOKEN_PRINT_VAR_INT:
            return std::string("PRINT_VAR_INT");
        case TokenType::TOKEN_PRINT_VAR_FLOAT:
            return std::string("PRINT_VAR_FLOAT");
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
    seps.insert({'!', Token::TokenType::TOKEN_BANG});
    seps.insert({'*', Token::TokenType::TOKEN_ASTERISK});
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
    keywords.insert({"main", Token::TokenType::TOKEN_MAIN});
    keywords.insert({"def", Token::TokenType::TOKEN_DEF});
    keywords.insert({"return", Token::TokenType::TOKEN_RETURN});
    keywords.insert({"set", Token::TokenType::TOKEN_SET});
    keywords.insert({"void", Token::TokenType::TOKEN_DES_VOID});
    keywords.insert({"int", Token::TokenType::TOKEN_DES_INT});
    keywords.insert({"float", Token::TokenType::TOKEN_DES_FLOAT});

    // fill built function
    keywords.insert({"printVarInt", 
                     Token::TokenType::TOKEN_PRINT_VAR_INT});
    keywords.insert({"printVarFloat", 
                     Token::TokenType::TOKEN_PRINT_VAR_FLOAT});
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
    if (code.eof())
    {
        tok = Token(Token::TokenType::TOKEN_EOF);
        return false;
    }

    // Parse the line
    parseLine(line);

    // Skip empty lines
    while (toks_per_line.size() == 0)
    {
        getline(code, line);
        if (code.eof())
        {
            tok = Token(Token::TokenType::TOKEN_EOF);
            return false;
        }

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
    // bool prev_token_arith = false;
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
            // TODO - Need to take care of negative sign
            // if (*iter != '-' || !prev_token_arith)
            // {
                std::string literal = cur_token_str;
                Token::TokenType type = sep_iter->second;
                Token _tok(type, literal);
                // std::cout << _tok.prinTokenType() << " | "
                //           << _tok.getVal() << "\n";
                toks_per_line.push(_tok);
                
                // if (isArithOpr(*iter))
                //     prev_token_arith = true;
                // else
                //     prev_token_arith = false;
                continue;
            // }
        }

        // prev_token_arith = false;
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

        if (isType<int>(cur_token_str))
        {
            Token::TokenType type = Token::TokenType::TOKEN_INT;
            Token _tok(type, cur_token_str);
            toks_per_line.push(_tok);
            continue;
        }
        else if (isType<float>(cur_token_str))
        {
            Token::TokenType type = Token::TokenType::TOKEN_FLOAT;
            Token _tok(type, cur_token_str);
            toks_per_line.push(_tok);
            continue;
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

/*
// I can't think of any good names for this one
// This function simply helps us to identif
bool Lexer::isArithOpr(char c)
{
    return (c == '+' || c == '-' || c == '*' || c == '/' || 
            c == '=' || c == ','); // The last two helps to 
    // identify x = -10 or add(1, -10)
}
*/
}
