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
#include <llvm/Support/CommandLine.h>
#include "corefn/corefn.h"
#include "passes.h"
#include "parser.h"
#include "nodes.h"
#include "lexer.h"
#include "codegen.h"
#include <chrono>
#include <filesystem>

#define DEBUG_PRINT_LLVM_IR false

using namespace std;
using namespace llvm;
using namespace t;

void TypeCheck();
void RunEntry();
void registerCoreFunctions(unique_ptr<orc::LLJIT>& JIT);

ExitOnError ExitOnErr;
vector<unique_ptr<Node>> FunctionDeclarations, TopLevelExpressions;
vector<unique_ptr<Structure>> Structures;
set<string> ImportedFiles;

// Command Line Options
cl::OptionCategory Category("Options");
cl::opt<string> FileName(cl::Positional, cl::Required, cl::desc("<input file>"), cl::cat(Category));

int main(int argc, char* argv[]) {
    cl::HideUnrelatedOptions(Category);
    cl::ParseCommandLineOptions(argc, argv);
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeLLVM();
    unique_ptr<Parser> parser = make_unique<Parser>();
    string absPath = filesystem::absolute(FileName.c_str()); // get absolute path relative to the cwd
    ImportedFiles.insert(absPath);
    parser->ParseFile(absPath, FunctionDeclarations, TopLevelExpressions, Structures, ImportedFiles);
    TypeCheck();
    RunEntry();
}

void TypeCheck(){
    Symbols.CreateScope();
    for (auto& structure : Structures) {
        structure->checkType();
    }
    for(auto& node : FunctionDeclarations){
        node->checkType();
    }
    for(auto& node : TopLevelExpressions){
        node->checkType();
    }
}

void RunEntry(){
    Symbols.Reset();
    auto entryFunction = make_unique<t::Function>("entry", move(make_shared<t::Type>("number")), FileLocation(),
                                                    vector<pair<shared_ptr<t::Type>,string>>(),
                                                    move(TopLevelExpressions));
    for(auto &Decl : FunctionDeclarations){
        auto IR = Decl->codegen();
    }
    for(auto &Decl : Structures){
        auto IR = Decl->codegen();
    }
    if(auto entry = entryFunction->codegen()){
        auto JIT = ExitOnErr(orc::LLJITBuilder().create());
        if (!JIT)
            exit(1);
        registerCoreFunctions(JIT);
        LoopAnalysisManager LAM;
        FunctionAnalysisManager FAM;
        CGSCCAnalysisManager CGAM;
        ModuleAnalysisManager MAM;

        PassBuilder PB;

        PB.registerModuleAnalyses(MAM);
        PB.registerCGSCCAnalyses(CGAM);
        PB.registerFunctionAnalyses(FAM);
        PB.registerLoopAnalyses(LAM);
        PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

        ModulePassManager MPM;

        if (DEBUG_PRINT_LLVM_IR)
            MPM.addPass(PrintModulePass());MPM.addPass(createModuleToFunctionPassAdaptor(RemoveEmptyBasicBlocksPass()));

        MPM.addPass(createModuleToFunctionPassAdaptor(RemoveAfterFirstTerminatorPass()));
        MPM.addPass(createModuleToFunctionPassAdaptor(SimplifyCFGPass()));
        MPM.addPass(createModuleToFunctionPassAdaptor(PromotePass()));
        MPM.run(*t::Module, MAM);
        if(verifyModule(*t::Module)) {  //make sure the module is safe to run
            cerr << "Can't run llvm Module, is faulty! \n";
            return;
        }
        if (auto Err = JIT.get()->addIRModule(
                orc::ThreadSafeModule(std::move(t::Module), std::move(Context)))){
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

void registerCoreFunctions(std::unique_ptr<orc::LLJIT>& JIT) {
    auto &jd = JIT->getMainJITDylib();
    auto &dl = JIT->getDataLayout();

    orc::MangleAndInterner Mangle(JIT->getExecutionSession(), dl);
    std::vector<std::pair<std::string, void*> > coreFnMap{
            std::make_pair("printString", (void*)printString),
            std::make_pair("printAscii", (void*)printAscii),
            std::make_pair("printNumber", (void*)printNumber),
            std::make_pair("input", (void*)input),
    };
    for(auto & [name, fn] : coreFnMap) {
        auto s = orc::absoluteSymbols({{ Mangle(name), JITEvaluatedSymbol(pointerToJITTargetAddress(fn), JITSymbolFlags::Exported)}});
        if(auto Error = jd.define(s)) {
            errs() << "Error defining symbols: " << toString(std::move(Error)) << "\n";
        }
    }
}