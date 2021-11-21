//
// Created by tommasopeduzzi on 12/08/2021.
//

#include "error.h"

void LogErrorLineNo(const std::string message){
    std::cerr << message << " on line " << "lineNo" << std::endl;
}

llvm::Value *LogError(const std::string message){
    std::cerr << message << std::endl;
    return nullptr;
}