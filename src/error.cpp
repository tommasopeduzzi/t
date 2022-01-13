//
// Created by tommasopeduzzi on 12/08/2021.
//

#include "error.h"

void LogErrorLineNo(const string message){
    cerr << message << " on line " << "lineNo" << endl;
}

llvm::Value *LogError(const string message){
    cerr << message << endl;
    return nullptr;
}