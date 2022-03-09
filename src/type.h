//
// Created by Tommaso Peduzzi on 06.01.22.
//

#pragma once

#include <string>
#include <memory>
#include <llvm/IR/Type.h>

using namespace std;
using namespace llvm;

namespace t {

    class Type {
    public:
        string type;
        shared_ptr<Type> subtype = nullptr;
        int size = 1;

        Type() = default;

        Type(const string type, int size = 1) : type(type), size(size) {}

        Type(const string type, unique_ptr<Type> subtype, int size) : type(type), subtype(move(subtype)), size(size) {}

        llvm::Type *GetLLVMType() const;

        //TODO: Unhardcode if type can be indexed
        bool isDynamicallyIndexable() { return type == "list" || type == "string"; }

        //TODO: Unhardcode if type is negatable
        bool isNegatable() { return type == "number" || type == "bool"; }
    };

}