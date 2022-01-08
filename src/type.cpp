//
// Created by Tommaso Peduzzi on 06.01.22.
//

#include <llvm/IR/Type.h>
#include "codegen.h"
#include "type.h"

llvm::Type* Type::GetLLVMType(){
    if (type == "number")
        return llvm::Type::getDoubleTy(*Context);
    else if (type == "string")
        return llvm::Type::getInt8PtrTy(*Context);
    else if (type == "boolean")
        return llvm::Type::getInt1Ty(*Context);
    else if (type == "void")
        return llvm::Type::getVoidTy(*Context);
    else
        assert(false && "Unknown type");
}