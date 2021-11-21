//
// Created by tommasopeduzzi on 12/08/2021.
//

#ifndef T_NODES_H
#define T_NODES_H

#include <memory>
#include <vector>
#include <string>
#include <llvm/IR/Value.h>

class Node{
public:
    virtual ~Node() = default;
    virtual llvm::Value *codegen() = 0;
};

class Negative : public Node {
    std::unique_ptr<Node>  Expression;
public:
    Negative(std::unique_ptr<Node> Expression) : Expression(std::move(Expression)) {}
    virtual llvm::Value *codegen();
};

class Number : public Node{
    double Value;
public:
    Number(const double value) : Value(value) {}
    virtual llvm::Value *codegen();
};

class Bool : public Node{
    bool Value;
public:
    Bool(const bool value) : Value(value) {}
    virtual llvm::Value *codegen();
};

class String : public Node{
    std::string Value;
public:
    String(std::string value) : Value(value) {}
    virtual llvm::Value *codegen();
};

class Variable : public Node{
public:
    Variable(const std::string name) : Name(name){}
    std::string Name;
    virtual llvm::Value *codegen();
};

class VariableDefinition : public Node {
    std::string Name, Type;
    std::unique_ptr<Node> Value;
public:
    VariableDefinition(std::string name, std::string type, std::unique_ptr<Node> Init) :
        Name(name), Type(type), Value(std::move(Init)) {}
    virtual llvm::Value *codegen();
};

class BinaryExpression : public Node{
    std::string Op;
    std::unique_ptr<Node> LHS, RHS;

public:
    BinaryExpression(std::string op, std::unique_ptr<Node> lhs,
                            std::unique_ptr<Node> rhs):
                            Op(op), LHS(move(lhs)), RHS(move(rhs)) {}
    virtual llvm::Value *codegen();
};

class IfExpression : public Node {
    std::unique_ptr<Node> Condition;
    std::vector<std::unique_ptr<Node>> Then, Else;
public:
    IfExpression(std::unique_ptr<Node> Cond, std::vector<std::unique_ptr<Node>> Then, std::vector<std::unique_ptr<Node>> Else) :
        Condition(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {};
    virtual llvm::Value *codegen();
};

class ForLoop : public Node {
    std::string VariableName;
    std::unique_ptr<Node> Start, Condition, Step;
    std::vector<std::unique_ptr<Node>> Body;
public:
    ForLoop(std::string VariableName, std::unique_ptr<Node> Start, std::unique_ptr<Node>Condition,
            std::unique_ptr<Node> Step, std::vector<std::unique_ptr<Node>> Body) : VariableName(VariableName),
                                                                                   Start(std::move(Start)), Condition(std::move(Condition)), Step(std::move(Step)),
                                                                                   Body(std::move(Body)) {};
    virtual llvm::Value *codegen();
};

class WhileLoop : public Node {
    std::unique_ptr<Node> Condition;
    std::vector<std::unique_ptr<Node>> Body;
public:
    WhileLoop(std::unique_ptr<Node> Condition, std::vector<std::unique_ptr<Node>> Body) : Condition(std::move(Condition)),
                                                                                   Body(std::move(Body)) {};
    virtual llvm::Value *codegen();
};

class Return : public Node {
    std::unique_ptr<Node> Expression;
public:
    Return(std::unique_ptr<Node> Expression) :
    Expression(std::move(Expression)) {};
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
    std::string Name, Type;
    std::vector<std::pair<std::string, std::string>> Arguments;
    std::vector<std::unique_ptr<Node>> Body;

public:
    Function(const std::string name, const std::string type,
             std::vector<std::pair<std::string, std::string>> arguments,
             std::unique_ptr<Node> body) :
            Name(name), Type(type), Arguments(move(arguments)) {
        Body.push_back(std::move(body));
    };
    Function(const std::string name, const std::string type,
             std::vector<std::pair<std::string, std::string>> arguments,
             std::vector<std::unique_ptr<Node>> body) :
            Name(name), Type(type), Arguments(move(arguments)), Body(move(body)) {}

    virtual llvm::Value *codegen();
};

class Extern : public Node {
    std::string  Name, Type;
    std::vector<std::pair<std::string, std::string>> Arguments;

public:
    Extern(const std::string name, const std::string type,
           std::vector<std::pair<std::string, std::string>> arguments) :
           Name(name), Type(type), Arguments(std::move(arguments)) {}
    virtual llvm::Value *codegen();
};

#endif //T_NODES_H
