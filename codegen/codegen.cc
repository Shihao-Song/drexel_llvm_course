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

    module->print(errs(), nullptr);
}
}
