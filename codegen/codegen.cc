#include "codegen/codegen.hh"

namespace Frontend
{
void Codegen::gen()
{
    context = std::make_unique<LLVMContext>();
    module = std::make_unique<Module>(mod_name, *context);

    // Create a new builder for the module.
    builder = std::make_unique<IRBuilder<>>(*context);

    // Codegen begins
    auto &program = parser->getProgram();
    auto &statements = program.getStatements();

    for (auto &statement : statements)
    {
        assert(statement->isStatementFunc());
        funcGen(statement.get());
        local_var_tracking.clear();
    }
}

void Codegen::funcGen(Statement *_statement)
{
    FuncStatement *func_statement = 
        static_cast<FuncStatement*>(_statement);

    auto& func_name = func_statement->getFuncName();
    auto& func_args = func_statement->getFuncArgs();
    auto& func_codes = func_statement->getFuncCodes();

    // IR Gen
    // Prepare argument types
    std::vector<Type *> ir_gen_func_args;
    for (auto &arg : func_args)
    {
        if (arg.isArgInt())
            ir_gen_func_args.push_back(Type::getInt32Ty(*context));
        else if (arg.isArgFloat())
            ir_gen_func_args.push_back(Type::getFloatTy(*context));
        else
            assert(false && 
                   "[Error] funcGen: unsupported argument type. \n");
    }

    // Prepare return type
    Type *ir_gen_ret_type;
    if (func_statement->isRetTypeVoid())
        ir_gen_ret_type = Type::getVoidTy(*context);
    else if (func_statement->isRetTypeInt())
        ir_gen_ret_type = Type::getInt32Ty(*context);
    else if (func_statement->isRetTypeFloat()) 
        ir_gen_ret_type = Type::getFloatTy(*context);
    else
        assert(false && 
               "[Error] funcGen: unsupported return type. \n");

    // Determine function type
    FunctionType *ir_gen_func_type =
        FunctionType::get(ir_gen_ret_type, ir_gen_func_args, false);

    // Determine linkage type.
    // So far, let's keep ExternalLinkage for all functions
    GlobalValue::LinkageTypes link_type = Function::ExternalLinkage;

    // Create function declaration
    Function *ir_gen_func = Function::Create(ir_gen_func_type,
                                             link_type, 
                                             func_name, 
                                             module.get());
   
    // Create a new basic block to start insertion into.
    BasicBlock *BB = BasicBlock::Create(*context, "", ir_gen_func);
    builder->SetInsertPoint(BB);

    // Generate the code section
    // (1) Allocate space for arguments
    std::cout << func_name << "\n";
    auto i = 0;
    for (auto &arg : ir_gen_func->args())
    {
        static auto &func_arg_types = parser->getFuncArgTypes(func_name);

        Value *val = &arg;
        Value *reg;

        if (func_arg_types[i] == Parser::TypeRecord::INT)
        {
            reg = builder->CreateAlloca(Type::getInt32Ty(*context));
            builder->CreateStore(val, reg);
	}
        else if (func_arg_types[i] == Parser::TypeRecord::FLOAT)
        {
            reg = builder->CreateAlloca(Type::getFloatTy(*context));
            builder->CreateStore(val, reg);
	}

        local_var_tracking[func_args[i].getLiteral()] = reg;
        i++;
    }

    // (2) Rest of the codes
    for (auto &statement : func_codes)
    {
        if (statement->isStatementSet())
        {
            setGen(func_name, statement.get());
        }
        else if (statement->isStatementBuiltin())
        {
            builtinGen(statement.get());
        }
        else if (statement->isStatementRet())
        {
            retGen(func_name, statement.get());
        }
    }

    // Verify function
    verifyFunction(*ir_gen_func);
}

void Codegen::setGen(std::string &cur_func_name,
                     Statement *_statement)
{
    SetStatement* set_statement = 
        static_cast<SetStatement*>(_statement);

    auto &iden = set_statement->getIden();
    auto expr = set_statement->getExpr();

    // Allocate for identifier
    Value *reg;
    if (auto iter = local_var_tracking.find(iden);
            iter == local_var_tracking.end())
    {
        if (parser->isInFuncVarInt(cur_func_name, iden))
            reg = builder->CreateAlloca(Type::getInt32Ty(*context));
        else if (parser->isInFuncVarFloat(cur_func_name, iden))
            reg = builder->CreateAlloca(Type::getFloatTy(*context));
    }
    else
    {
        reg = iter->second;
    }

    Value *val;

    if (expr->isExprLiteral())
    {
        LiteralExpression* lit = static_cast<LiteralExpression*>(expr);

        val = literalExprGen(cur_func_name, iden, lit);
    }
    else if (expr->isExprArith())
    {
        ArithExpression *arith = static_cast<ArithExpression *>(expr);

        val = arithExprGen(cur_func_name, iden, arith);        
    }
    else if (expr->isExprCall())
    {
        CallExpression *call = static_cast<CallExpression*>(expr);

        val = callExprGen(call);
    }
    
    builder->CreateStore(val, reg);
    local_var_tracking[iden] = reg;
}

// built-ins are implemented in util/
// please check bc_compile_and_run.bash and util for more info
void Codegen::builtinGen(Statement *_statement)
{
    static FunctionCallee printVarInt = 
        module->getOrInsertFunction("printVarInt",
            Type::getVoidTy(*context), 
            Type::getInt32Ty(*context));

    static FunctionCallee printVarFloat = 
        module->getOrInsertFunction("printVarFloat",
            Type::getVoidTy(*context), 
            Type::getFloatTy(*context));
   
    BuiltinCallStatement *built_in_statement = 
        static_cast<BuiltinCallStatement*>(_statement);

    auto call_expr = built_in_statement->getCallExpr();
    assert(call_expr->isExprCall());

    auto &func_name = call_expr->getCallFunc();
    auto args = call_expr->getArgNames();
    assert(args.size() == 1);

    auto iter = local_var_tracking.find(args[0]);
    assert(iter != local_var_tracking.end());

    if (func_name == "printVarInt")
    {
        Value *val = builder->CreateLoad(Type::getInt32Ty(*context),
                                         iter->second);
        builder->CreateCall(printVarInt, val);
    }
    else if (func_name == "printVarFloat")
    {
        Value *val = builder->CreateLoad(Type::getFloatTy(*context),
                                         iter->second);
        builder->CreateCall(printVarFloat, val);
    }
}

void Codegen::retGen(std::string &cur_func_name,
                     Statement *_statement)
{
    RetStatement* ret = static_cast<RetStatement*>(_statement);

    auto &ret_val_str = ret->getLiteral();

    Value *ret_val;
    if (auto iter = local_var_tracking.find(ret_val_str);
            iter != local_var_tracking.end())
    {
        if (parser->isInFuncVarInt(cur_func_name, ret_val_str))
        {
            ret_val = builder->CreateLoad(Type::getInt32Ty(*context),
                                          iter->second);
        } 
        else if (parser->isInFuncVarFloat(cur_func_name, ret_val_str))
        {
            ret_val = builder->CreateLoad(Type::getFloatTy(*context),
                                          iter->second);
        }    
    }
    else
    {
        if (ret->isLitInt())
        {
            ret_val = ConstantInt::get(*context, APInt(32, stoi(ret_val_str)));
        }
        else if (ret->isLitFloat())
        {
            ret_val = ConstantFP::get(*context, APFloat(stof(ret_val_str)));
        }
    }
    builder->CreateRet(ret_val);
}

Value* Codegen::arithExprGen(std::string& cur_func_name, 
                             std::string& iden, 
                             ArithExpression* arith)
{
    Value *val_left = nullptr;
    Value *val_right = nullptr;

    // Recursively generate the left arith expr
    if (arith->getLeft() != nullptr)
    {
        Expression *next_expr = arith->getLeft();
        if (next_expr->isExprArith())
        {
            ArithExpression* next_arith = 
                static_cast<ArithExpression*>(next_expr);
            val_left = arithExprGen(cur_func_name, 
                                    iden,
                                    next_arith);
        }
    }

    // Recursively generate the right arith expr
    if (arith->getRight() != nullptr)
    {
        Expression *next_expr = arith->getRight();
        if (next_expr->isExprArith())
        {
            ArithExpression* next_arith = 
                static_cast<ArithExpression*>(next_expr);
            val_right = arithExprGen(cur_func_name, 
                                    iden,
                                    next_arith);
        }
    }

    if (val_left == nullptr)
    {
        Expression *left_expr = arith->getLeft();

        // The left_expr must either be literal or call
        assert((left_expr->isExprLiteral() || 
                left_expr->isExprCall() ));

        if (left_expr->isExprLiteral())
        {
            LiteralExpression* lit = 
                static_cast<LiteralExpression*>(left_expr);

            val_left = literalExprGen(cur_func_name, iden, lit);
        }
    }

    if (val_right == nullptr)
    {
        Expression *right_expr = arith->getRight();

        // The right_expr must either be literal or call
	assert((right_expr->isExprLiteral() || 
                right_expr->isExprCall() ));

        if (right_expr->isExprLiteral())
        {
            LiteralExpression* lit = 
                static_cast<LiteralExpression*>(right_expr);

            val_right = literalExprGen(cur_func_name, iden, lit);
        }
    }

    // Generate operators
    auto opr = arith->getOperator();
    bool int_opr = parser->isInFuncVarInt(cur_func_name, iden);

    switch (opr) 
    {
        case '+':
            if (int_opr)
                return builder->CreateAdd(val_left, val_right);
            else
                return builder->CreateFAdd(val_left, val_right);
        case '-':
            if (int_opr)
                return builder->CreateSub(val_left, val_right);
            else
                return builder->CreateFSub(val_left, val_right);
        case '*':
            if (int_opr)
                return builder->CreateMul(val_left, val_right);
            else
                return builder->CreateFMul(val_left, val_right);
        case '/':
            if (int_opr)
                return builder->CreateSDiv(val_left, val_right);
            else
                return builder->CreateFDiv(val_left, val_right);
    }
}

Value* Codegen::literalExprGen(std::string& cur_func_name, 
                               std::string& iden, 
                               LiteralExpression* lit)
{
    Value *val = nullptr;

    auto iter = local_var_tracking.find(lit->getLiteral());

    if (iter == local_var_tracking.end())
    {
        assert((lit->isLiteralInt() || 
                lit->isLiteralFloat()));

        auto val_str = lit->getLiteral();
        if (lit->isLiteralInt())
        {
            val = ConstantInt::get(*context, APInt(32, stoi(val_str)));
        }
        else if (lit->isLiteralFloat())
        {
            val = ConstantFP::get(*context, APFloat(stof(val_str)));
        }
    }
    else
    {
        if (parser->isInFuncVarInt(cur_func_name, iden))
        {
            val = builder->CreateLoad(Type::getInt32Ty(*context),
                                      iter->second);
            
        }
        else if (parser->isInFuncVarFloat(cur_func_name, iden))
        {
            val = builder->CreateLoad(Type::getFloatTy(*context),
                                      iter->second);
            
        }
    }

    assert(val != nullptr);
    return val;
}

Value* Codegen::callExprGen(CallExpression *call)
{
    auto &def = call->getCallFunc();
    Function *call_func = module->getFunction(def);
    if (!call_func)
    {
        std::cerr << "[Error] Please define function before CALL\n";
        exit(0);
    }

    auto args = call->getArgNames();
    auto arg_types = parser->getFuncArgTypes(def);
    assert(args.size() == call_func->arg_size());
    assert(arg_types.size() == call_func->arg_size());

    std::vector<Value*> call_func_args;
    for (auto i = 0; i < call_func->arg_size(); i++)
    {
        if (auto iter = local_var_tracking.find(args[i]);
                iter != local_var_tracking.end())
        {
            Value *val; 
            if (arg_types[i] == Parser::TypeRecord::INT)
            {
                val = builder->CreateLoad(Type::getInt32Ty(*context),
                                          iter->second);
            }
            else if (arg_types[i] == Parser::TypeRecord::FLOAT)
            {
                val = builder->CreateLoad(Type::getFloatTy(*context),
                                          iter->second);
            }
            call_func_args.push_back(val);
        }
        else
        {
            if (arg_types[i] == Parser::TypeRecord::INT)
            {
                call_func_args.push_back(ConstantInt::get(*context, 
                    APInt(32, stoi(args[i]))));
            }
            else if (arg_types[i] == Parser::TypeRecord::FLOAT)
            {
                call_func_args.push_back(ConstantFP::get(*context, 
                    APFloat(stof(args[i]))));
            }
        }
    }

    return builder->CreateCall(call_func, call_func_args);
}

void Codegen::print()
{
    module->print(errs(), nullptr);
    std::error_code EC;
    raw_fd_ostream out(out_fn, EC);
    WriteBitcodeToFile(*module, out);
}
}
