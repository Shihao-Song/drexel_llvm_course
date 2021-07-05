#ifndef __CODEGEN_HH__
#define __CODEGEN_HH__

#include "parser/parser.hh"

// LLVM IR codegen libraries
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Bitcode/BitcodeWriter.h"

using namespace llvm;

namespace Frontend
{
class Codegen
{
  protected:
    std::unique_ptr<LLVMContext> context;
    std::unique_ptr<Module> module;
    std::unique_ptr<IRBuilder<>> builder;

    std::string mod_name;
    std::string out_fn;

    Parser* parser;

  public:

    Codegen(const char* _mod_name,
            const char* _out_fn)
        : mod_name(_mod_name)
        , out_fn(_out_fn)
    {}

    void setParser(Parser *_parser)
    {
        parser = _parser;
    }

    void gen();

    void print();

  protected:
    std::unordered_map<std::string, Value *> local_var_tracking;

    void funcGen(Statement *);
    void assnGen(std::string &,Statement *);
    void builtinGen(Statement *);
    void callGen(Statement *);
    void retGen(std::string &,Statement *);

    Value* allocaForLitIden(std::string&,
                            std::string&,
                            Parser::TypeRecord&,
                            LiteralExpression*,
                            ArrayExpression*);
    // Value* allocaForIdxIden(IndexExpression*);
    
    void arrayExprGen(Parser::TypeRecord,
                      Value*,
                      ArrayExpression*);

    Value* arithExprGen(Parser::TypeRecord,ArithExpression*);

    Value* literalExprGen(Parser::TypeRecord, LiteralExpression*);

    Value* callExprGen(CallExpression*);
};
}

#endif
