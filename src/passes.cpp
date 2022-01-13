//
// Created by Tommaso Peduzzi on 03.11.21.
//

#include "passes.h"
#include "nodes.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

PreservedAnalyses RemoveAfterFirstTerminatorPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
    SmallVector<Instruction * > ToBeErased;
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

PreservedAnalyses RemoveEmptyBasicBlocksPass::run(Function &F,
                                                              FunctionAnalysisManager &AM) {
    EliminateUnreachableBlocks(F);
    SmallVector<BasicBlock*> ToBeRemoved;
    for(auto &BB : F){
        if(BB.empty())
            ToBeRemoved.push_back(&BB);
    }
    for(auto *BB : ToBeRemoved){
        BB->eraseFromParent();
    }
    return PreservedAnalyses::all();
}