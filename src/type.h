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
    int size = 1;
    Type() = default;
    Type(const std::string type, int size) : type(type), size(size){}
    Type(const std::string type, std::unique_ptr<Type> subtype, int size) : type(type), subtype(move(subtype)), size(size) {}
    llvm::Type* GetLLVMType();
};
#endif