//
// Created by tommasopeduzzi on 12/08/2021.
//
#pragma once

#include <string>
#include "nodes.h"
#include <iostream>

namespace t{
    llvm::Value *LogError(const FileLocation location, string message);
    llvm::Value *LogError(string message);
}
