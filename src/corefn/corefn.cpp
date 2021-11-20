//
// Created by Tommaso Peduzzi on 20.11.21.
//

#include <llvm/Support/raw_ostream.h>
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "corefn.h"

extern "C" double printString(const char* str){
    llvm::outs() << str;
    return 0;
}

extern "C" double printAscii(double c){
    llvm::outs() << (char)c;
    return 0;
}