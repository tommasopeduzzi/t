//
// Created by Tommaso Peduzzi on 03.11.21.
//

#include "passes.h"
#include "nodes.h"

llvm::PreservedAnalyses llvm::RemoveAfterFirstTerminatorPass::run(Function &F,
                                      llvm::FunctionAnalysisManager &AM) {
    for(llvm::Function::iterator b = F.begin(), end = F.end(); b != end; ++b){
        BasicBlock &BB = *b;
        errs() << BB.getName() << "\n";
    }
    return PreservedAnalyses::all();
}
