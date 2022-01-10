#include <memory>
#include <iostream>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/Mangling.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRPrintingPasses.h>
#include "corefn/corefn.h"
#include "passes.h"
#include "parser.h"
#include "nodes.h"
#include "lexer.h"
#include "codegen.h"
#include <chrono>


void RunEntry();

llvm::ExitOnError ExitOnErr;
std::vector<std::unique_ptr<Node>> FunctionDeclarations, TopLevelExpressions;
std::set<std::string> ImportedFiles;

void registerCoreFunctions(std::unique_ptr<llvm::orc::LLJIT>& JIT) {
    auto &jd = JIT->getMainJITDylib();
    auto &dl = JIT->getDataLayout();

    llvm::orc::MangleAndInterner Mangle(JIT->getExecutionSession(), dl);
    std::vector<std::pair<std::string, void*> > coreFnMap{
            std::make_pair("printString", (void*)printString),
            std::make_pair("printAscii", (void*)printAscii),
            std::make_pair("printNumber", (void*)printNumber),
            std::make_pair("input", (void*)input),
    };
    for(auto & [name, fn] : coreFnMap) {
        auto s = llvm::orc::absoluteSymbols({{ Mangle(name), llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(fn), llvm::JITSymbolFlags::Exported)}});
        if(auto Error = jd.define(s)) {
            llvm::errs() << "Error defining symbols: " << llvm::toString(std::move(Error)) << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    if(argc < 2){
        std::cerr << "Expected File as first argument!\n" << argc;
        return 0;
    }
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    InitializeLLVM();
    std::unique_ptr<Parser> parser = std::make_unique<Parser>();
    ImportedFiles.insert(argv[1]);
    parser->ParseFile(argv[1], FunctionDeclarations, TopLevelExpressions, ImportedFiles);
    RunEntry();
}

void RunEntry(){
    auto entryFunction = std::make_unique<Function>("entry", std::move(std::make_unique<Type>("number", 0)),
                                                    std::vector<std::pair<std::unique_ptr<Type>,std::string>>(),
                                                    std::move(TopLevelExpressions));
    for(auto &Decl : FunctionDeclarations){
        if(auto IR = Decl->codegen()){
            //IR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    }
    if(auto entry = entryFunction->codegen()){
        auto JIT = ExitOnErr(llvm::orc::LLJITBuilder().create());
        //entry->print(llvm::errs());
        if (!JIT)
            exit(1);
        registerCoreFunctions(JIT);
        llvm::LoopAnalysisManager LAM;
        llvm::FunctionAnalysisManager FAM;
        llvm::CGSCCAnalysisManager CGAM;
        llvm::ModuleAnalysisManager MAM;

        llvm::PassBuilder PB;

        PB.registerModuleAnalyses(MAM);
        PB.registerCGSCCAnalyses(CGAM);
        PB.registerFunctionAnalyses(FAM);
        PB.registerLoopAnalyses(LAM);
        PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

        llvm::ModulePassManager MPM; // TODO: figure out a way to add the default pipeline after the fact.
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::RemoveEmptyBasicBlocksPass()));
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::RemoveAfterFirstTerminatorPass()));
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::SimplifyCFGPass()));
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::PromotePass()));
        // MPM.addPass(llvm::PrintModulePass());
        MPM.run(*Module, MAM);
        if(llvm::verifyModule(*Module)) {  //make sure the module is safe to run
            std::cerr << "Can't run llvm Module, is faulty! \n";
            return;
        }
        if (auto Err = JIT.get()->addIRModule(
                llvm::orc::ThreadSafeModule(std::move(Module), std::move(Context)))){
            std::cerr << "Error loading module: " << toString(std::move(Err)) << "\n";
            return;
        }
        auto EntrySym = JIT->lookup("entry");
        if (!EntrySym){
            std::cerr << "Error loading entry-function!\n";
            return;
        }
        auto *Expr = (double(*)())EntrySym->getAddress();
        auto exitCode = Expr();
        exit(exitCode);
    }
}