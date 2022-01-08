//
// Created by Tommaso Peduzzi on 22.10.21.
//

#ifndef T_CODEGEN_H
#define T_CODEGEN_H

#include <llvm/IR/IRBuilder.h>

extern std::unique_ptr<llvm::LLVMContext> Context;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::unique_ptr<llvm::Module> Module;
extern std::vector<std::map<std::string, llvm::AllocaInst *>> Variables;

void InitializeLLVM();
#endif //T_CODEGEN_H
