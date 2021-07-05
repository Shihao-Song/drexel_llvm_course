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
    // TODO - introduce pointer
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
    auto i = 0;
    std::vector<Parser::TypeRecord> func_arg_types;
    if (ir_gen_func->arg_size())
        func_arg_types = parser->getFuncArgTypes(func_name);
    for (auto &arg : ir_gen_func->args())
    {
        Value *val = &arg;
        Value *reg;

        // TODO - introduce pointer
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
        if (statement->isStatementAssn())
        {
            assnGen(func_name, statement.get());
        }
        else if (statement->isStatementBuiltinCall())
        {
            builtinGen(statement.get());
        }
        else if (statement->isStatementRet())
        {
            retGen(func_name, statement.get());
        }
        else if (statement->isStatementNormalCall())
        {
            callGen(statement.get());
        }
    }

    if (func_statement->isRetTypeVoid())
    {
        Value *val = nullptr;
        builder->CreateRet(val);
    }

    // Verify function
    verifyFunction(*ir_gen_func);
}

void Codegen::assnGen(std::string &cur_func_name,
                      Statement *_statement)
{
    AssnStatement* assn_statement = 
        static_cast<AssnStatement*>(_statement);

    auto iden = assn_statement->getIden();
    auto expr = assn_statement->getExpr();

    // Allocate for identifier
    std::string var_name;
    Parser::TypeRecord var_type;
    Value *reg;

    // TODO, support index
    if (iden->isExprLiteral())
    {
        LiteralExpression *lit = 
            static_cast<LiteralExpression*>(iden);

        ArrayExpression* array_info =
            (expr->isExprArray()) ?
            static_cast<ArrayExpression*>(expr) :
            nullptr;

        reg = allocaForLitIden(cur_func_name, var_name, var_type, 
                               lit, array_info);
    }
    else
    {
        std::cerr << "[Error] unsupported identifier type. \n";
        exit(0);
    }
    
    Value *val = nullptr;
    if (expr->isExprLiteral())
    {
        LiteralExpression* lit = static_cast<LiteralExpression*>(expr);

        val = literalExprGen(var_type, lit);
    }
    else if (expr->isExprArray())
    {
        ArrayExpression* array_info = static_cast<ArrayExpression*>(expr);
        arrayExprGen(var_type, reg, array_info);
    }
    else if (expr->isExprArith())
    {
        ArithExpression *arith = static_cast<ArithExpression *>(expr);

        val = arithExprGen(var_type, arith);        
    }
    else if (expr->isExprCall())
    {
        CallExpression *call = static_cast<CallExpression*>(expr);

        val = callExprGen(call);
    }
   
    if (val != nullptr) 
        builder->CreateStore(val, reg);
    local_var_tracking[var_name] = reg;
}

Value* Codegen::allocaForLitIden(std::string &func_name,
                                 std::string &var_name, 
                                 Parser::TypeRecord &var_type,
                                 LiteralExpression* lit,
                                 ArrayExpression* array_info)
{
    var_name = lit->getLiteral();
    var_type = parser->getInFuncVarType(func_name, var_name);

    Value *reg;
    if (auto iter = local_var_tracking.find(var_name);
            iter == local_var_tracking.end())
    {
        if (var_type == Parser::TypeRecord::INT)
        {
            reg = builder->CreateAlloca(Type::getInt32Ty(*context));
        }
        else if (var_type == Parser::TypeRecord::FLOAT)
        {
            reg = builder->CreateAlloca(Type::getFloatTy(*context));
        }
        else if (var_type == Parser::TypeRecord::INT_ARRAY || 
                 var_type == Parser::TypeRecord::FLOAT_ARRAY)
        {
            assert(array_info != nullptr);

            // Extract number of elements
            auto num_ele_expr = array_info->getNumElements();
            assert(num_ele_expr->isExprLiteral());

            auto num_ele_lit = 
                static_cast<LiteralExpression*>(num_ele_expr);
            assert(num_ele_lit->isLiteralInt());
    
            auto num_ele_int = stoi(num_ele_lit->getLiteral());

            // Get array type
            Type *ele_type = (var_type == Parser::TypeRecord::INT_ARRAY) ?
                             Type::getInt32Ty(*context) :
                             Type::getFloatTy(*context);

            ArrayType* array_type = ArrayType::get(ele_type, num_ele_int);

            reg = builder->CreateAlloca(array_type);

        }
        else
        {
	    std::cerr << "[Error] unsupported allocation type for "
                      << var_name << "\n";
            exit(0);
        }
    }
    else
    {
        reg = iter->second;
    }

    return reg;
}

// built-ins are implemented in util/ and linked through llvm-link
// please check bc_compile_and_run.bash and util for more info
// This one is bit different from our callGen implementation
// since we are defining printVarInt/printVarFloat at current
// compilation unit.
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
    
    CallStatement *built_in_statement = 
        static_cast<CallStatement*>(_statement);

    auto call_expr = built_in_statement->getCallExpr();
    assert(call_expr->isExprCall());

    auto &func_name = call_expr->getCallFunc();
    auto &func_args = call_expr->getArgs();
    assert(func_args.size() == 1);
    auto expr = func_args[0].get();

    Parser::TypeRecord var_type = (func_name == "printVarInt") ? 
        Parser::TypeRecord::INT : Parser::TypeRecord::FLOAT;
    Value *val;
    if (expr->isExprLiteral())
    {
        LiteralExpression *lit = 
            static_cast<LiteralExpression*>(expr);

        val = literalExprGen(var_type, lit);
    }
    else if (expr->isExprArith())
    {
        ArithExpression *arith = 
            static_cast<ArithExpression*>(expr);

        val = arithExprGen(var_type, arith);
    }
    else if (expr->isExprCall())
    {
        CallExpression *call = 
            static_cast<CallExpression*>(expr);

        val = callExprGen(call);
    }
    else if (expr->isExprIndex())
    {
        std::vector<Value *> index;
        index.push_back(ConstantInt::get(*context, APInt(32, 0)));
        index.push_back(ConstantInt::get(*context, APInt(32, 1)));
        auto base = builder->CreateInBoundsGEP(local_var_tracking.find("x")->second,
                                               index);

        val = builder->CreateLoad(Type::getInt32Ty(*context), base);
    }

    if (func_name == "printVarInt")
    {
        builder->CreateCall(printVarInt, val);
    }
    else if (func_name == "printVarFloat")
    {
        builder->CreateCall(printVarFloat, val);
    }
}

void Codegen::callGen(Statement *_statement)
{
     CallStatement *built_in_statement = 
        static_cast<CallStatement*>(_statement);

    auto call_expr = built_in_statement->getCallExpr();
    assert(call_expr->isExprCall());

    callExprGen(call_expr);
}

void Codegen::retGen(std::string &cur_func_name,
                     Statement *_statement)
{
    RetStatement* ret = static_cast<RetStatement*>(_statement);

    auto expr = ret->getRetVal();

    Parser::TypeRecord ret_type = parser->getFuncRetType(cur_func_name);

    Value *val;
    if (expr->isExprLiteral())
    {
        LiteralExpression *lit = 
            static_cast<LiteralExpression*>(expr);

        val = literalExprGen(ret_type, lit);
    }
    else if (expr->isExprArith())
    {
        ArithExpression *arith = 
            static_cast<ArithExpression*>(expr);

        val = arithExprGen(ret_type, arith);
    }
    else if (expr->isExprCall())
    {
        CallExpression *call = 
            static_cast<CallExpression*>(expr);

        val = callExprGen(call);
    }

    builder->CreateRet(val);
}

void Codegen::arrayExprGen(Parser::TypeRecord array_type,
                           Value *reg,
                           ArrayExpression* array_info)
{
    // Determine element type
    Parser::TypeRecord type;
    if (array_type == Parser::TypeRecord::INT_ARRAY)
        type = Parser::TypeRecord::INT;
    else if (array_type == Parser::TypeRecord::FLOAT_ARRAY)
        type = Parser::TypeRecord::FLOAT;
    else
        assert(false);

    // Get the 0th array element
    // This actually took me a long time to figure out, looks like
    // the first index will get you the pointer, then the second
    // index gets you to the first element.
    std::vector<Value *> index;
    index.push_back(ConstantInt::get(*context, APInt(32, 0)));
    index.push_back(ConstantInt::get(*context, APInt(32, 0)));
    auto base = builder->CreateInBoundsGEP(reg, index);

    auto cnt = 0;
    auto last_ele_idx = array_info->getElements().size() - 1;
    auto const_one = ConstantInt::get(*context, APInt(32, 1));
    for (auto ele : array_info->getElements())
    {
        Value *val;
        if (ele->isExprLiteral())
        {
            LiteralExpression *lit = 
                static_cast<LiteralExpression*>(ele.get());

            val = literalExprGen(type, lit);
        }
        else if (ele->isExprArith())
        {
            ArithExpression *arith = 
                static_cast<ArithExpression*>(ele.get());

            val = arithExprGen(type, arith);
        }
        else if (ele->isExprCall())
        {
            CallExpression *call = 
                static_cast<CallExpression*>(ele.get());

            val = callExprGen(call);
        }

        builder->CreateStore(val, base);
        if (++cnt <= last_ele_idx)
        {
            // increment one to the base
            base = builder->CreateInBoundsGEP(base, const_one); 
        }
    }
}

Value* Codegen::arithExprGen(Parser::TypeRecord type, 
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
            val_left = arithExprGen(type, 
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
            val_right = arithExprGen(type,
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

            val_left = literalExprGen(type, lit);
        }
        else if (left_expr->isExprCall())
        {
            CallExpression* call = 
                static_cast<CallExpression*>(left_expr);

            val_left = callExprGen(call);
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

            val_right = literalExprGen(type, lit);
        }
        else if (right_expr->isExprCall())
        {
            CallExpression* call = 
                static_cast<CallExpression*>(right_expr);

            val_right = callExprGen(call);
	}
    }

    assert(val_left != nullptr);
    assert(val_right != nullptr);

    // Generate operators
    auto opr = arith->getOperator();

    switch (opr) 
    {
        case '+':
            if (type == Parser::TypeRecord::INT)
                return builder->CreateAdd(val_left, val_right);
            else if (type == Parser::TypeRecord::FLOAT)
                return builder->CreateFAdd(val_left, val_right);
        case '-':
            if (type == Parser::TypeRecord::INT)
                return builder->CreateSub(val_left, val_right);
            else if (type == Parser::TypeRecord::FLOAT)
                return builder->CreateFSub(val_left, val_right);
        case '*':
            if (type == Parser::TypeRecord::INT)
                return builder->CreateMul(val_left, val_right);
            else if (type == Parser::TypeRecord::FLOAT)
                return builder->CreateFMul(val_left, val_right);
        case '/':
            if (type == Parser::TypeRecord::INT)
                return builder->CreateSDiv(val_left, val_right);
            else if (type == Parser::TypeRecord::FLOAT)
                return builder->CreateFDiv(val_left, val_right);
    }
}

Value* Codegen::literalExprGen(Parser::TypeRecord type, 
                               LiteralExpression* lit)
{
    Value *val;
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
        if (type == Parser::TypeRecord::INT)
        {
            val = builder->CreateLoad(Type::getInt32Ty(*context),
                                      iter->second);
            
        }
        else if (type == Parser::TypeRecord::FLOAT)
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

    auto args = call->getArgs();
    auto arg_types = parser->getFuncArgTypes(def);
    assert(args.size() == call_func->arg_size());
    assert(arg_types.size() == call_func->arg_size());

    std::vector<Value*> call_func_args;
    for (auto i = 0; i < call_func->arg_size(); i++)
    {
        auto expr = args[i].get();

        Value *val;
        if (expr->isExprLiteral())
        {
            LiteralExpression *lit = 
                static_cast<LiteralExpression*>(expr);

            val = literalExprGen(arg_types[i], lit);
        }
        else if (expr->isExprArith())
        {
            ArithExpression *arith = 
                static_cast<ArithExpression*>(expr);

            val = arithExprGen(arg_types[i], arith);
        }
        else if (expr->isExprCall())
        {
            CallExpression *call = 
                static_cast<CallExpression*>(expr);

            val = callExprGen(call);
        }
  
        call_func_args.push_back(val);
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
