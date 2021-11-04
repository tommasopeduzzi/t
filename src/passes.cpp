//
// Created by Tommaso Peduzzi on 03.11.21.
//

#include "passes.h"
#include "nodes.h"

llvm::PreservedAnalyses llvm::RemoveAfterFirstTerminatorPass::run(Function &F,
                                      llvm::FunctionAnalysisManager &AM) {
    for(BasicBlock &BB : F){
        bool isTerminated = false;
        for(Instruction &I : BB){
            if(isTerminated)
                I.removeFromParent();
            else
                isTerminated = I.isTerminator();
        }
    }
    return PreservedAnalyses::all();
}
