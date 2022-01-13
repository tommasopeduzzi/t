//
// Created by tommasopeduzzi on 12/08/2021.
//

#ifndef T_ERROR_H
#define T_ERROR_H

#include <string>
#include "nodes.h"
#include <iostream>

llvm::Value *LogError(const string message);
void LogErrorLineNo(const string message);
#endif //T_ERROR_H
