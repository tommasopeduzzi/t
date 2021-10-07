//
// Created by tommasopeduzzi on 12/08/2021.
//

#include "lexer.h"
#include "error.h"

void LogErrorLineNo(const std::string message){
    std::cerr << message << " on line " << lineNo << std::endl;
}

void LogError(const std::string message){
    std::cerr << message << std::endl;
}