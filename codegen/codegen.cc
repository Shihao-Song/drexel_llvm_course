#include "parser/parser.hh"

namespace Frontend
{
void Parser::codegen(const char* fn)
{
    context = std::make_unique<LLVMContext>();
    module = std::make_unique<Module>(fn, *context);

    // Create a new builder for the module.
    builder = std::make_unique<IRBuilder<>>(*context);

    // Codegen begins

    module->print(errs(), nullptr);
}
}
