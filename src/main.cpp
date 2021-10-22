#include <iostream>
#include "parser.h"
#include "nodes.h"
#include "lexer.h"
#include "codegen.h"

void HandleExpression();

void HandleExternDeclaration();

void HandleFunctionDefinition();

int main() {
//    llvm::InitializeNativeTarget();
//    llvm::InitializeNativeTargetAsmPrinter();
//    llvm::InitializeNativeTargetAsmParser();
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
    if(auto Expression = ParseTopLevelExpression()){
        if(auto ExpressionIR = Expression->codegen()){
            ExpressionIR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    }
    else{
        getNextToken();
    }
}


