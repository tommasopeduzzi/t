//
// Created by Tommaso Peduzzi on 03.11.21.
//

#ifndef T_PASSES_H
#define T_PASSES_H
#include "nodes.h"
namespace llvm {

class RemoveAfterFirstTerminatorPass : public PassInfoMixin<RemoveAfterFirstTerminatorPass> {
    public:
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    };

} // namespace llvm

#endif //T_PASSES_H
