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
#include <llvm/Support/Host.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/CommandLine.h>
#include "corefn/corefn.h"
#include "passes.h"
#include "parser.h"
#include "nodes.h"
#include "lexer.h"
#include "codegen.h"
#include <chrono>
#include <filesystem>

using namespace std;
using namespace llvm;
using namespace t;

ExitOnError ExitOnErr;
vector<unique_ptr<Node>> FunctionDeclarations, TopLevelExpressions;
vector<unique_ptr<Structure>> Structures;
set<string> ImportedFiles;

// Command Line Options
cl::OptionCategory Category("Options");
cl::opt<string> FileName(cl::Positional, cl::Required, cl::desc("<input file>"), cl::cat(Category));
cl::opt<bool> JIT("jit", cl::desc("Choose if program should be JIT-compiled"), cl::cat(Category));
cl::opt<bool> EmitIR("emit-llvm", cl::desc("Emit LLVM IR for "), cl::cat(Category));

int main(int argc, char *argv[]) {
    cl::HideUnrelatedOptions(Category);
    cl::ParseCommandLineOptions(argc, argv);

    // Parse File
    unique_ptr<Parser> parser = make_unique<Parser>();
    string absPath = filesystem::absolute(FileName.c_str());
    ImportedFiles.insert(absPath);
    parser->ParseFile(absPath, FunctionDeclarations, TopLevelExpressions, Structures, ImportedFiles);

    // Check Types
    Symbols.CreateScope();
    for (auto &structure: Structures) {
        structure->checkType();
    }
    for (auto &node: FunctionDeclarations) {
        node->checkType();
    }
    for (auto &node: TopLevelExpressions) {
        node->checkType();
    }
    Symbols.Reset();

    //Initialize LLVM for codegen
    // TODO: Figure out linking with all llvm components for initializing all targets
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeLLVM();

    // setup Target
    // TODO: Change target based on user input
    auto TargetTriple = sys::getDefaultTargetTriple();
    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);
    if (!Target) {
        errs() << Error;
        return 1;
    }
    auto CPU = "generic";
    auto Features = "";
    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
    auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    t::Module->setDataLayout(TargetMachine->createDataLayout());
    t::Module->setTargetTriple(TargetTriple);

    // Create Entry Function
    auto entryFunction = make_unique<t::Function>("main", move(make_shared<t::Type>("number")), FileLocation(),
                                                  vector<pair<shared_ptr<t::Type>, string>>(),
                                                  move(TopLevelExpressions));

    // Codegen Function and Structure-Declarations
    for (auto &Decl: FunctionDeclarations) {
        auto IR = Decl->codegen();
    }
    for (auto &Decl: Structures) {
        auto IR = Decl->codegen();
    }

    // Codegen Entry Function
    auto entry = entryFunction->codegen();
    if (!entry)
        return 1;

    // Preapare and Run Pass Manager
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

    if (EmitIR)
        MPM.addPass(PrintModulePass());

    MPM.addPass(createModuleToFunctionPassAdaptor(RemoveEmptyBasicBlocksPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(RemoveAfterFirstTerminatorPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(SimplifyCFGPass()));
    MPM.addPass(createModuleToFunctionPassAdaptor(PromotePass()));
    MPM.run(*t::Module, MAM);

    // Verify Correctness of Module
    if (verifyModule(*t::Module)) {
        cerr << "LLVM Module faulty. Use '--emit-llvm' to debug. \n";
        return 1;
    }

    // Create And Run JIT
    if (JIT) {
        auto JIT = ExitOnErr(orc::LLJITBuilder().create());
        if (!JIT)
            exit(1);

        // Register Core Functions
        // TODO: Link shared functions using a shared library
        auto &jd = JIT->getMainJITDylib();
        auto &dl = JIT->getDataLayout();

        orc::MangleAndInterner Mangle(JIT->getExecutionSession(), dl);
        std::vector<std::pair<std::string, void *> > coreFnMap{
                std::make_pair("printString", (void *) printString),
                std::make_pair("printAscii", (void *) printAscii),
                std::make_pair("printNumber", (void *) printNumber),
                std::make_pair("input", (void *) input),
                std::make_pair("isEqual", (void *) isEqual),
        };
        for (auto &[name, fn]: coreFnMap) {
            auto s = orc::absoluteSymbols(
                    {{Mangle(name), JITEvaluatedSymbol(pointerToJITTargetAddress(fn), JITSymbolFlags::Exported)}});
            if (auto Error = jd.define(s)) {
                errs() << "Error defining symbols: " << toString(std::move(Error)) << "\n";
            }
        }

        if (auto Err = JIT.get()->addIRModule(
                orc::ThreadSafeModule(std::move(t::Module), std::move(Context)))) {
            std::cerr << "Error loading module: " << toString(std::move(Err)) << "\n";
            return 1;
        }
        auto EntrySym = JIT->lookup("main");
        if (!EntrySym) {
            std::cerr << "Error loading entry-function!\n";
            return 1;
        }
        auto *Expr = (double (*)()) EntrySym->getAddress();
        auto exitCode = Expr();
        exit(exitCode);
    }
    else {
        auto Filename = "output.o";
        std::error_code EC;
        raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);
        if (EC) {
            errs() << "Could not open file: " << EC.message();
            return 1;
        }
        legacy::PassManager pass;
        auto FileType = CGFT_ObjectFile;

        if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
            errs() << "TargetMachine can't emit a file of this type";
            return 1;
        }

        pass.run(*t::Module);
        dest.flush();

    }

}