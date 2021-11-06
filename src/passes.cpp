//
// Created by Tommaso Peduzzi on 03.11.21.
//

#include "passes.h"
#include "nodes.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

llvm::PreservedAnalyses llvm::RemoveAfterFirstTerminatorPass::run(Function &F,
                                      llvm::FunctionAnalysisManager &AM) {
    llvm::SmallVector<llvm::Instruction * > ToBeErased;
    for(BasicBlock &BB : F){
        bool isTerminated = false;
        for(Instruction &I : BB){
            if(isTerminated)
                ToBeErased.push_back(&I);
            else
                isTerminated = I.isTerminator();
        }
    }
    for(auto *I : ToBeErased){
        I->eraseFromParent();
    }
    return PreservedAnalyses::all();
}

llvm::PreservedAnalyses llvm::RemoveEmptyBasicBlocksPass::run(Function &F,
                                                              llvm::FunctionAnalysisManager &AM) {
    llvm::EliminateUnreachableBlocks(F);
    /*llvm::SmallVector<llvm::BasicBlock*> ToBeRemoved;
    for(auto &BB : F){
        if(BB.empty())
            ToBeRemoved.push_back(&BB);
    }
    for(auto &BB : ToBeRemoved) {
        BB->eraseFromParent();
    }*/
    return PreservedAnalyses::all();
}