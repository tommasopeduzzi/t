#include <memory>
#include "parser.h"
#include "nodes.h"
#include "lexer.h"
#include "codegen.h"
#include <iostream>
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "passes.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include "llvm/IR/IRPrintingPasses.h"
void HandleExpression();

void HandleExternDeclaration();

void HandleFunctionDefinition();

void RunEntry();

llvm::ExitOnError ExitOnErr;
llvm::BasicBlock* entryBlock;
std::vector<std::unique_ptr<Node>> TopLevelExpressions;

int main() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    InitializeLLVM();
    getNextToken(); // get the first token
    while (true) {
        switch (CurrentToken) {
            case eof:
                RunEntry();
                return 0;
            case def:
                HandleFunctionDefinition();
                break;
            case ext:
                HandleExternDeclaration();
                break;
            default:
                HandleExpression();
                break;
        }
    }
}

void RunEntry(){
    auto entryFunction = std::make_unique<Function>("entry",
                                                    std::vector<std::string>(),
                                                    std::move(TopLevelExpressions));
    if(auto entry = entryFunction->codegen()){
        auto JIT = ExitOnErr(llvm::orc::LLJITBuilder().create());
        entry->print(llvm::errs());
        if (!JIT)
            return;

        llvm::LoopAnalysisManager LAM;
        llvm::FunctionAnalysisManager FAM;
        llvm::CGSCCAnalysisManager CGAM;
        llvm::ModuleAnalysisManager MAM;


        llvm::PassBuilder PB;

        // FAM.registerPass([&] { return PB.buildDefaultAAPipeline(); });

        PB.registerModuleAnalyses(MAM);
        PB.registerCGSCCAnalyses(CGAM);
        PB.registerFunctionAnalyses(FAM);
        PB.registerLoopAnalyses(LAM);
        PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

        llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::PassBuilder::OptimizationLevel::O2);
        MPM.addPass(llvm::PrintModulePass());
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::RemoveAfterFirstTerminatorPass()));
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::SimplifyCFGPass()));
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::PromotePass()));
        MPM.run(*Module, MAM);

        if(llvm::verifyModule(*Module)) {  //make sure the module is safe to run
            std::cerr << "Can't run llvm Module, is faulty! \n";
            return;
        }
        if (auto Err = JIT.get()->addIRModule(
                llvm::orc::ThreadSafeModule(std::move(Module), std::move(Context))))
            return;
        auto EntrySym = JIT->lookup("entry");
        if (!EntrySym)
            return;

        auto *Expr = (double(*)())EntrySym->getAddress();

        fprintf(stderr, "Evaluated to %f\n", Expr());
    }
}

void HandleFunctionDefinition() {
    if(auto Function = ParseFunction()){
        if(auto FunctionIR = Function->codegen()){
            FunctionIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    }
    else{
        getNextToken();
    }
}

void HandleExternDeclaration() {
    if(auto Extern = ParseExtern()){
        if(auto ExternIR = Extern->codegen()){
            ExternIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    }
    else{
        getNextToken();
    }
}

void HandleExpression() {
    if(auto Expression = ParseExpression()){
        TopLevelExpressions.push_back(std::move(Expression));
    }
    else{
        getNextToken();
    }
}