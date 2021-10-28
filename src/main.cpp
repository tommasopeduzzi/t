#include <memory>
#include "parser.h"
#include "nodes.h"
#include "lexer.h"
#include "codegen.h"

void HandleExpression();

void HandleExternDeclaration();

void HandleFunctionDefinition();
llvm::ExitOnError ExitOnErr;
int main() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    InitializeLLVM();
    getNextToken(); // get the first token
    while (true) {
        switch (CurrentToken) {
            case eof:
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
gith    if(auto Expression = ParseTopLevelExpression()){
        if(auto ExpressionIR = Expression->codegen()){
            // Try to detect the host arch and construct an LLJIT instance.
            auto JIT = ExitOnErr(llvm::orc::LLJITBuilder().create());

            if (!JIT)
                return;

            if (auto Err = JIT.get()->addIRModule(
                    llvm::orc::ThreadSafeModule(std::move(Module), std::move(Context))))
                return;

            auto EntrySym = JIT->lookup("__anon_expr");
            if (!EntrySym)
                return;

            auto *Expr = (double(*)())EntrySym->getAddress();

            fprintf(stderr, "Evaluated to %f\n", Expr());
        }
    }
    else{
        getNextToken();
    }
}


