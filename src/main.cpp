#include <iostream>
#include "parser.h"
#include "nodes.h"

int main() {
    std::vector<std::unique_ptr<Node>> Program = ParseProgram();
    InitializeModule();
    for(auto& Node : Program){
        if(auto IR = Node->codegen()){
            IR->print(llvm::errs());
            fprintf(stderr, "\n");
        }
    }
}
