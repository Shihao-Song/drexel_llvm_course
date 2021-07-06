#include "parser/parser.hh"

namespace Frontend
{
Parser::Parser(const char* fn) : lexer(new Lexer(fn))
{
    // Pre-load all the tokens
    lexer->getToken(cur_token);
    lexer->getToken(next_token);

    // Fill the pre-built 
    std::vector<TypeRecord> arg_types;
    TypeRecord ret_type = TypeRecord::VOID;
    FuncRecord record;

    // printVarInt
    arg_types.push_back(TypeRecord::INT);
    record.ret_type = ret_type;
    record.arg_types = arg_types;
    record.is_built_in = true;
    func_def_tracker.insert({"printVarInt", record});

    // printVarFloat
    arg_types.clear();
    arg_types.push_back(TypeRecord::FLOAT);
    record.ret_type = ret_type;
    record.arg_types = arg_types;
    record.is_built_in = true;
    func_def_tracker.insert({"printVarFloat", record});

    parseProgram();
}

void Parser::advanceTokens()
{
    cur_token = next_token;
    lexer->getToken(next_token);
}

void Parser::parseProgram()
{
    // Should always be functions at this time since
    // we don't support globals or structures...
    while (!cur_token.isTokenEOF())
    {
        FuncStatement::RetType ret_type;
        std::unique_ptr<Identifier> iden;
        std::vector<FuncStatement::Argument> args;
        std::vector<std::shared_ptr<Statement>> codes;

        // determine return type
        if (cur_token.isTokenDesVoid())
        {
	    ret_type = FuncStatement::RetType::VOID;
        }
        else if (cur_token.isTokenDesInt())
        {
	    ret_type = FuncStatement::RetType::INT;
        }
        else if (cur_token.isTokenDesFloat())
        {
	    ret_type = FuncStatement::RetType::FLOAT;
        }
        else
        {
	    std::cerr << "[Error] parseProgram: unsupported return type\n";
            std::cerr << "[Line] " << cur_token.getLine() << "\n";
	    exit(0);
        }
                
        // function name
        advanceTokens();
        iden = std::make_unique<Identifier>(cur_token);
        if (!next_token.isTokenLP())
        {
            std::cerr << "[Error] Don't support global variables or "
                      << "class or structure yet.\n";
            std::cerr << "[Line] " << cur_token.getLine() << "\n";
            exit(0);
	}

        advanceTokens();
        assert(cur_token.isTokenLP());

        // extract arguments
        // TODO, argument types need to support pointer
        while (!cur_token.isTokenRP())
        {
            advanceTokens();
            if (cur_token.isTokenRP()) break; // no args

            std::string arg_type = cur_token.getLiteral();

            advanceTokens();
            std::unique_ptr<Identifier> iden(new Identifier(cur_token));
            FuncStatement::Argument arg(arg_type, iden);
            args.push_back(arg);

            recordLocalVars(arg);

            advanceTokens();
        }
        assert(cur_token.isTokenRP());

        advanceTokens();
        assert(cur_token.isTokenLBrace());

        // record function def
        recordDefs(iden->getLiteral(), ret_type, args);

        // parse the codes section
        while (!cur_token.isTokenRBrace())
        {
            advanceTokens();
            if (cur_token.isTokenReturn())
            {
                advanceTokens();

                cur_expr_type = getFuncRetType(iden->getLiteral());

                auto ret = parseExpression();

                std::unique_ptr<RetStatement> ret_statement = 
                    std::make_unique<RetStatement>(ret);

                codes.push_back(std::move(ret_statement));
            }
            else
            {
                // is it a normal function call?
                if (auto [is_def, is_built_in] = 
                        isFuncDef(cur_token.getLiteral());
                    is_def)
                {
                    Statement::StatementType call_type = is_built_in ?
                        Statement::StatementType::BUILT_IN_CALL_STATEMENT :
                        Statement::StatementType::NORMAL_CALL_STATEMENT;

                    auto code = parseCall();
                    std::unique_ptr<CallStatement> call = 
                        std::make_unique<CallStatement>(code, call_type); 

                    codes.push_back(std::move(call));

                    continue;
                }

                // is it a variable-assignment?
                if (isTokenTypeKeyword(cur_token) ||
                    cur_token.isTokenIden())
                {
                    auto code = parseAssnStatement();
                    // code->printStatement();
                    codes.push_back(std::move(code));

                    continue;
                }
            }
        }

        per_func_var_tracking.insert({iden->getLiteral(),
                                      PerFuncRecord(local_var_type_tracker)});
        local_var_type_tracker.clear();

        std::unique_ptr<Statement> func_proto
            (new FuncStatement(ret_type, 
                               iden, 
                               args, 
                               codes));
        program.addStatement(func_proto);
        
        advanceTokens();
    }
}

std::unique_ptr<Statement> Parser::parseAssnStatement()
{
    if (isTokenTypeKeyword(cur_token))
    {
        Token type_token = cur_token;

        advanceTokens();
        if (auto var_iter = local_var_type_tracker.find(cur_token.getLiteral());
                var_iter != local_var_type_tracker.end())
        {
            std::cerr << "[Error] Re-definition of "
                      << cur_token.getLiteral() << "\n";
            std::cerr << "[Line] " << cur_token.getLine() << "\n";
            exit(0);
        }

        bool is_array = (next_token.isTokenLBracket()) ? 
                        true : false;

        recordLocalVars(cur_token, type_token, is_array);

        std::unique_ptr<Expression> iden =
            std::make_unique<LiteralExpression>(cur_token);

	std::unique_ptr<Expression> expr;
        if (!is_array)
        {
            advanceTokens();
            assert(cur_token.isTokenEqual());

            advanceTokens();
            expr = parseExpression();
        }
        else
        {
            expr = parseArrayExpr();
        }
	
        std::unique_ptr<Statement> statement = 
            std::make_unique<AssnStatement>(iden, expr);

        return statement;
    }
    else
    {
        auto var_iter = local_var_type_tracker.find(cur_token.getLiteral());
        if (var_iter == local_var_type_tracker.end())
        {
            std::cerr << "[Error] Undefined variable of "
                      << cur_token.getLiteral() << "\n";
            std::cerr << "[Line] " << cur_token.getLine() << "\n";
            exit(0);
        }

        cur_expr_type = TypeRecord::MAX;
        auto iden = parseExpression();

        assert(cur_token.isTokenEqual());
        advanceTokens();

	std::unique_ptr<Expression> expr;
        if (var_iter->second == TypeRecord::INT_ARRAY || 
            var_iter->second == TypeRecord::FLOAT_ARRAY)
        {
            cur_expr_type = (var_iter->second == TypeRecord::INT_ARRAY)
                            ? TypeRecord::INT
                            : TypeRecord::FLOAT;
            
	}
        else
        {
            cur_expr_type = var_iter->second;
        }

        expr = parseExpression();
        
        std::unique_ptr<Statement> statement = 
            std::make_unique<AssnStatement>(iden, expr);

        return statement;
    }
}

std::unique_ptr<Expression> Parser::parseArrayExpr()
{
    advanceTokens();
    assert(cur_token.isTokenLBracket());

    advanceTokens();
    // num_ele must be an integer
    auto swap = cur_expr_type;
    cur_expr_type = TypeRecord::INT;
    auto num_ele = parseExpression();
    cur_expr_type = swap;
    if (!(num_ele->isExprLiteral()))
    {
        std::cerr << "[Error] Number of array elements "
                  << "must be a single integer. \n"
                  << "[Line] " << cur_token.getLine() << "\n";
        exit(0);
    }
    auto num_ele_lit = static_cast<LiteralExpression*>(num_ele.get());
    if (!(num_ele_lit->isLiteralInt()))
    {
        std::cerr << "[Error] Number of array elements "
                  << "must be a single integer. \n"
                  << "[Line] " << cur_token.getLine() << "\n";
        exit(0);
    }
    int num_eles_int = stoi(num_ele_lit->getLiteral());
    if (num_eles_int <= 1)
    {
        std::cerr << "[Error] Number of array elements "
                  << "must be larger than 1. \n"
                  << "[Line] " << cur_token.getLine() << "\n";
        exit(0);
    }

    assert(cur_token.isTokenRBracket());

    advanceTokens();
    assert(cur_token.isTokenEqual());

    advanceTokens();
    assert(cur_token.isTokenLBrace());

    std::vector<std::shared_ptr<Expression>> eles;
    if (!next_token.isTokenRBrace())
    {
        advanceTokens();
        while (!cur_token.isTokenRBrace())
        {
            eles.push_back(parseExpression());
            if (cur_token.isTokenComma())
                advanceTokens();
        }

        // We make sure consistent number of elements
        if (num_eles_int != eles.size())
        {
            std::cerr << "[Error] Accpeted format: "
                      << "(1) pre-allocation style - array<int> x[10] = {} "
                      << "(2) #initials == #elements - "
                      << "array<int> x[2] = {1, 2} \n"
                      << "[Line] " << cur_token.getLine() << "\n";
            exit(0);
        }
    }
    else
    {
        advanceTokens();
    }

    advanceTokens();

    std::unique_ptr<Expression> ret = 
        make_unique<ArrayExpression>(num_ele, eles);

    return ret;
}

std::unique_ptr<Expression> Parser::parseExpression()
{
    std::unique_ptr<Expression> left = parseTerm();

    while (true)
    {
        if (cur_token.isTokenPlus() || 
            cur_token.isTokenMinus())
        {
            Expression::ExpressionType expr_type;
            if (cur_token.isTokenPlus())
            {
                expr_type = Expression::ExpressionType::PLUS;
            }
            else
            {
                expr_type = Expression::ExpressionType::MINUS;
            }

            advanceTokens();

            std::unique_ptr<Expression> right;

            // Priority one. ()
            if (cur_token.isTokenLP())
            {
                right = parseTerm();
                left = std::make_unique<ArithExpression>(left, 
                       right, 
                       expr_type);
                continue;
            }
            
            // Priority two. *, /
            // check if the next token is a function call
	    std::unique_ptr<Expression> pending_expr = nullptr;
            if (auto [is_def, is_built_in] = 
                    isFuncDef(cur_token.getLiteral());
                    is_def)
            {
                strictTypeCheck(cur_token);
                pending_expr = parseCall();
            }

            // check if the next token is an array index
            // TODO - add deref in the future
            if (bool is_index = (next_token.isTokenLBracket()) ?
                                true : false;
                is_index)
            {
                assert(pending_expr == nullptr);
                strictTypeCheck(cur_token, is_index);
                pending_expr = parseIndex();
            }

            if (next_token.isTokenAsterisk() ||
                next_token.isTokenSlash())
            {
                if (pending_expr != nullptr)
                {
                    advanceTokens();
                    right = parseTerm(std::move(pending_expr));
                }
                else
                {
                    right = parseTerm();
                }
            }
            else
            {
                if (pending_expr != nullptr)
                {
                    right = std::move(pending_expr);
                }
                else
                {
                    strictTypeCheck(cur_token);
                    right = std::make_unique<LiteralExpression>(cur_token);
                }
                advanceTokens();
            }

            left = std::make_unique<ArithExpression>(left, 
                       right, 
                       expr_type);
        }
        else
        {
            return left;
        }
    }
}

// For Div/Mul
std::unique_ptr<Expression> Parser::parseTerm(
    std::unique_ptr<Expression> pending_left)
{   
    std::unique_ptr<Expression> left = 
        (pending_left != nullptr) ? std::move(pending_left) : parseFactor();

    while (true)
    {
        if (cur_token.isTokenAsterisk() || 
            cur_token.isTokenSlash())
        {
            Expression::ExpressionType expr_type;
            if (cur_token.isTokenAsterisk())
            {
                expr_type = Expression::ExpressionType::ASTERISK;
            }
            else
            {
                expr_type = Expression::ExpressionType::SLASH;
            }

            advanceTokens();

            std::unique_ptr<Expression> right;

            // We are trying to mul/div something with higher priority
            if (cur_token.isTokenLP()) 
            {
                right = parseTerm();
            }
            else
            {
                // TODO - add deref in the future
                bool is_index = (next_token.isTokenLBracket()) ?
                                true : false;

                strictTypeCheck(cur_token, is_index);
    
                if (is_index)
                    right = parseIndex();
                else if (auto [is_def, is_built_in] = 
                            isFuncDef(cur_token.getLiteral());
                            is_def)
                    right = parseCall();
                else
                    right = std::make_unique<LiteralExpression>(cur_token);

                advanceTokens();
            }

            left = std::make_unique<ArithExpression>(left, 
                       right, 
                       expr_type);

        }
        else
        {
            break;
        }

    }

    return left;
}

// Deal with () here
std::unique_ptr<Expression> Parser::parseFactor()
{
    std::unique_ptr<Expression> left;

    if (cur_token.isTokenLP())
    {
        advanceTokens();
        left = parseExpression();
        assert(cur_token.isTokenRP()); // Error checking
        advanceTokens();
        return left;
    }

    // TODO - add deref in the future
    bool is_index = (next_token.isTokenLBracket()) ?
                    true : false;

    strictTypeCheck(cur_token, is_index);
    
    if (is_index)
        left = parseIndex();
    else if (auto [is_def, is_built_in] = 
                 isFuncDef(cur_token.getLiteral());
                 is_def)
        left = parseCall();
    else
        left = std::make_unique<LiteralExpression>(cur_token);

    advanceTokens();

    return left;
}

std::unique_ptr<Expression> Parser::parseIndex()
{
    std::unique_ptr<Identifier> iden(new Identifier(cur_token));

    advanceTokens();
    assert(cur_token.isTokenLBracket());

    advanceTokens();

    // Index must be an integer
    auto swap = cur_expr_type;
    cur_expr_type = TypeRecord::INT;
    auto idx = parseExpression();
    cur_expr_type = swap;

    std::unique_ptr<Expression> ret = 
        std::make_unique<IndexExpression>(iden, idx);

    assert(cur_token.isTokenRBracket());

    return ret;
}

std::unique_ptr<Expression> Parser::parseCall()
{
    std::unique_ptr<Identifier> def(new Identifier(cur_token));

    advanceTokens();
    assert(cur_token.isTokenLP());

    advanceTokens();
    std::vector<std::shared_ptr<Expression>> args;

    auto &arg_types = getFuncArgTypes(def->getLiteral());
    unsigned idx = 0;
    while (!cur_token.isTokenRP())
    {
        if (cur_token.isTokenRP())
            break;

        auto swap = cur_expr_type;
        cur_expr_type = arg_types[idx++];
        args.push_back(parseExpression());
        cur_expr_type = swap;

        if (cur_token.isTokenRP())
            break;
        advanceTokens();
    }

    std::unique_ptr<CallExpression> ret = 
        std::make_unique<CallExpression>(def, args);

    return ret;
}

void RetStatement::printStatement()
{
    std::cout << "    {\n";
    std::cout << "      [Return]\n";
    if (ret->getType() == Expression::ExpressionType::LITERAL)
    {
        std::cout << "      " << ret->print(4);
    }
    else
    {
        std::cout << ret->print(4);
    }
    std::cout << "    }\n";
}

void AssnStatement::printStatement()
{
    std::cout << "    {\n";
    if (iden->getType() == Expression::ExpressionType::LITERAL)
    {
        std::cout << "      " << iden->print(4);
    }
    else
    {
        std::cout << iden->print(4);
    }

    std::cout << "      =\n";
    if (expr->getType() == Expression::ExpressionType::LITERAL)
    {
        std::cout << "      " << expr->print(4);
    }
    else
    {
        std::cout << expr->print(4);
    }

    std::cout << "    }\n";
}

void FuncStatement::printStatement()
{
    std::cout << "{\n";
    std::cout << "  Function Name: " << iden->print() << "\n";
    std::cout << "  Return Type: ";
    if (func_type == RetType::VOID)
    {
        std::cout << "void\n";
    }
    else if (func_type == RetType::INT)
    {
        std::cout << "int\n";
    }
    else if (func_type == RetType::FLOAT)
    {
        std::cout << "float\n";
    }

    std::cout << "  Arguments\n";
    for (auto &arg : args)
    {
        std::cout << "    " << arg.print() << "\n";
    }
    if (!args.size()) std::cout << "    NONE\n";

    std::cout << "  Codes\n";
    std::cout << "  {\n";
    for (auto &code : codes)
    {
        code->printStatement();
    }
    std::cout << "  }\n";
    std::cout << "}\n";
}
}
