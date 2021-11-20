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
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "corefn/corefn.h"

void RunEntry();

llvm::ExitOnError ExitOnErr;
std::vector<std::unique_ptr<Node>> FunctionDeclarations, TopLevelExpressions;
std::set<std::string> ImportedFiles;

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
    auto entryFunction = std::make_unique<Function>("entry", "number",
                                                    std::vector<std::pair<std::string,std::string>>(),
                                                    std::move(TopLevelExpressions));
    for(auto &Decl : FunctionDeclarations){
        if(auto IR = Decl->codegen()){
            //IR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    }
    if(auto entry = entryFunction->codegen()){
        auto JIT = ExitOnErr(llvm::orc::LLJITBuilder().create());
        // entry->print(llvm::errs());
        if (!JIT)
            return;
        auto &dl = JIT->getDataLayout();
        llvm::orc::MangleAndInterner Mangle(JIT->getExecutionSession(), dl);
        auto &jd = JIT->getMainJITDylib();

        auto s = llvm::orc::absoluteSymbols({{ Mangle("printString"), llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(&printString), llvm::JITSymbolFlags::Exported)}});
        jd.define(s);
        s = llvm::orc::absoluteSymbols({{ Mangle("printAscii"), llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(&printAscii), llvm::JITSymbolFlags::Exported)}});
        jd.define(s);

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

        llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::PassBuilder::OptimizationLevel::O2);
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::RemoveEmptyBasicBlocksPass()));
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::RemoveAfterFirstTerminatorPass()));
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::SimplifyCFGPass()));
        MPM.addPass(llvm::createModuleToFunctionPassAdaptor(llvm::PromotePass()));
        //MPM.addPass(llvm::PrintModulePass());
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
        exit(Expr());
    }
}