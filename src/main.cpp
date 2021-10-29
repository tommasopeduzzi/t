#include <memory>
#include "parser.h"
#include "nodes.h"
#include "lexer.h"
#include "codegen.h"

void HandleExpression();

void HandleExternDeclaration();

void HandleFunctionDefinition();

void RunEntry();

llvm::ExitOnError ExitOnErr;
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