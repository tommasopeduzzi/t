//
// Created by Tommaso Peduzzi on 22.10.21.
//

#ifndef T_CODEGEN_H
#define T_CODEGEN_H

#include <llvm/IR/IRBuilder.h>
#include "symbols.h"

extern std::unique_ptr<llvm::LLVMContext> Context;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::unique_ptr<llvm::Module> Module;
extern Symbols Symbols;

void InitializeLLVM();
#endif //T_CODEGEN_H
