//
// Created by Tommaso Peduzzi on 06.01.22.
//
#ifndef T_TYPE_H
#define T_TYPE_H
#include <string>
#include <llvm/IR/Type.h>

class Type {
public:
    const std::string type;
    std::unique_ptr<Type> subtype = nullptr;
    Type() = default;
    Type(const std::string type) : type(type) {}
    Type(const std::string type, std::unique_ptr<Type> subtype) : type(type), subtype(move(subtype)) {}
    llvm::Type* GetLLVMType();
};
#endif