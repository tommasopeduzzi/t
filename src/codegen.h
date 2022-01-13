//
// Created by Tommaso Peduzzi on 22.10.21.
//

#pragma once

#include <llvm/IR/IRBuilder.h>
#include "symbols.h"

using namespace std;
using namespace llvm;

namespace t {

    extern unique_ptr<LLVMContext> Context;
    extern unique_ptr<IRBuilder<>> Builder;
    extern unique_ptr<Module> Module;
    extern Symbols Symbols;

    void InitializeLLVM();
}