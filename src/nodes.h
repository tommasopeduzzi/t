//
// Created by tommasopeduzzi on 12/08/2021.
//

#ifndef T_NODES_H
#define T_NODES_H

#include <memory>
#include <vector>
#include <string>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

class Node{
public:
    virtual ~Node() = default;
    virtual llvm::Value *codegen() = 0;
    bool returnValue = false;
};

class Number : public Node{
    double Value;
public:
    Number(const double value) : Value(value) {}
    virtual llvm::Value *codegen();
};

class Variable : public Node{
public:
    Variable(const std::string name) : Name(name){}
    std::string Name;
    virtual llvm::Value *codegen();
};

class VariableDefinition : public Node {
    std::string Name;
    std::unique_ptr<Node> Value;
public:
    VariableDefinition(std::string name, std::unique_ptr<Node> Init) :
        Name(name), Value(std::move(Init)) {}
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
    std::vector<std::unique_ptr<Node>> Body;

public:
    Function(const std::string name,
             std::vector<std::string> arguments,
             std::unique_ptr<Node> body) :
            Name(name), Arguments(move(arguments)) {
        Body.push_back(std::move(body));
    };
    Function(const std::string name,
             std::vector<std::string> arguments,
             std::vector<std::unique_ptr<Node>> body) :
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
