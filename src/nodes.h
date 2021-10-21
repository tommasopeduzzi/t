//
// Created by tommasopeduzzi on 12/08/2021.
//

#ifndef T_NODES_H
#define T_NODES_H

#include <memory>
#include <vector>
#include <string>
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

void InitializeModule();

class Node{
public:
    virtual ~Node() {}
    virtual llvm::Value *codegen() = 0;
};

class Number : public Node{
    double Value;
public:
    Number(const double value) : Value(value) {}
    virtual llvm::Value *codegen();
};

class Variable : public Node{
    std::string Name;

public:
    Variable(const std::string name) : Name(name){}
    virtual llvm::Value *codegen();
};

class BinaryExpression : public Node{
    char Op;
    std::unique_ptr<Node> LHS, RHS;

public:
    BinaryExpression(char op, std::unique_ptr<Node> lhs,
                            std::unique_ptr<Node> rhs):
                            Op(op), LHS(move(lhs)), RHS(move(rhs)) {}
    virtual llvm::Value *codegen();
};

class Call : public Node{
    std::string Callee;
    std::vector<std::unique_ptr<Node>> Arguments;

public:
    Call(const std::string callee, std::vector<std::unique_ptr<Node>> arguments) :
    Callee(callee), Arguments(move(arguments)){}
    virtual llvm::Value *codegen();
};

class Function : public Node{
    std::string Name;
    std::vector<std::string> Arguments;
    std::unique_ptr<Node> Body;

public:
    Function(const std::string name,
             std::vector<std::string> arguments,
             std::unique_ptr<Node> body) :
            Name(name), Arguments(move(arguments)), Body(move(body)) {}
    virtual llvm::Value *codegen();
};

class Extern : public Node {
    std::string  Name;
    std::vector<std::string> Arguments;

public:
    Extern(const std::string name,
           std::vector<std::string> arguments) :
           Name(name), Arguments(std::move(arguments)) {}
    virtual llvm::Value *codegen();
};

#endif //T_NODES_H
