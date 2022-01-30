//
// Created by tommasopeduzzi on 12/08/2021.
//
#pragma once

#include "error.h"
#include "lexer.h"

#define RED     "\033[31m"      /* Red */
#define YELLOW  "\033[33m"      /* Yellow */
#define RESET   "\033[0m"

namespace t {
    llvm::Value *LogError(const FileLocation location, const string message) {
        cout << location.file << ":" << location.line <<":" << location.column << ": "
            << RED << "Error: "<< RESET<<  message << endl;
        return nullptr;
    }
}