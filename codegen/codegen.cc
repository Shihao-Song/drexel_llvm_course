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

    if (func_name != "main") return;

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
    // TODO-Shihao, understand different types
    // So far, let's keep ExternalLinkage for all functions
    GlobalValue::LinkageTypes link_type = Function::ExternalLinkage;

    // Create function declaration
    Function *ir_gen_func = Function::Create(ir_gen_func_type,
                                             link_type, 
                                             func_name, 
                                             module.get());
   
    // TODO, track all the arguments.

    // Create a new basic block to start insertion into.
    BasicBlock *BB = BasicBlock::Create(*context, "", ir_gen_func);
    builder->SetInsertPoint(BB);

    // Generate the code section
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
    }

    // TODO-tmp ret
    Value *ret_val = ConstantInt::get(*context, APInt(32, 0));
    builder->CreateRet(ret_val);

    // Verify function
    verifyFunction(*ir_gen_func);
}

void Codegen::setGen(std::string &cur_func_name,
                     Statement *_statement)
{
    SetStatement* set_statement = 
        static_cast<SetStatement*>(_statement);

    auto &iden = set_statement->getIden();
    auto &expr = set_statement->getExpr();

    if (expr->isExprLiteral())
    {
        auto val_str = expr->getLiteral();

        if (parser->isInFuncVarInt(cur_func_name, iden))
	{
            Value *reg = builder->CreateAlloca(Type::getInt32Ty(*context));
            Value *val = ConstantInt::get(*context, APInt(32, stoi(val_str)));
            builder->CreateStore(val, reg);
            local_var_tracking[iden] = reg;
        }
	else if (parser->isInFuncVarFloat(cur_func_name, iden))
	{
            Value *reg = builder->CreateAlloca(Type::getFloatTy(*context));
            Value *val = ConstantFP::get(*context, APFloat(stof(val_str)));
            builder->CreateStore(val, reg);
            local_var_tracking[iden] = reg;
        }
    }

    auto type = parser->isInFuncVarInt(cur_func_name, iden);
}

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

void Codegen::print()
{
    module->print(errs(), nullptr);
    std::error_code EC;
    raw_fd_ostream out(out_fn, EC);
    WriteBitcodeToFile(*module, out);
}
}
