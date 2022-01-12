//
// Created by tommasopeduzzi on 12/08/2021.
//

#ifndef T_NODES_H
#define T_NODES_H

#include "type.h"
#include <memory>
#include <vector>
#include <string>
#include <llvm/IR/Value.h>

enum NodeType{
    UNKNOWN,
    EXPRESSION,
    STATEMENT,
    NEGATIVE,
    NUMBER,
    BOOL,
    STRING,
    VARIABLE,
    INDEXING,
    BINARY_EXPRESSION,
    CALL,
    VARIABLE_DEFINITION,
    IF_STATEMENT,
    FOR_LOOP,
    WHILE_LOOP,
    RETURN,
    FUNCTION,
    EXTERN
};

class Node{
public:
    std::shared_ptr<Type> type;
    virtual ~Node() = default;
    Node() = default;
    Node(std::unique_ptr<Type> type) : type(move(type)) {}
    virtual NodeType getNodeType() const { return NodeType::UNKNOWN; }
    virtual llvm::Value *codegen() = 0;
};

class Expression : public Node{
public:
    virtual NodeType getNodeType() const { return NodeType::STATEMENT; }
    virtual llvm::Value *codegen() = 0;
    virtual std::pair<llvm::Value*, llvm::Type*> getAddressAndType() {assert(false && "SOMETHING WENT TERRIBLY WRONG");}
};

class Statement : public Node{
public:
    virtual NodeType getNodeType() const { return NodeType::STATEMENT; }
    Statement() = default;
    Statement(std::unique_ptr<Type> type) : Node(move(type)) {}
    virtual llvm::Value *codegen() = 0;
};

// TODO: maybe have another subclass for literals?

class Number : public Expression{
    double Value;
public:
    virtual NodeType getNodeType() const { return NodeType::NUMBER; }
    Number(const double value) : Value(value) {}
    virtual llvm::Value *codegen();
};

class Bool : public Expression{
    bool Value;
public:
    virtual NodeType getNodeType() const { return NodeType::BOOL; }
    Bool(const bool value) : Value(value) {}
    virtual llvm::Value *codegen();
};

class String : public Expression{
    std::string Value;
public:
    virtual NodeType getNodeType() const { return NodeType::STRING; }
    String(std::string value) : Value(value) {}
    virtual llvm::Value *codegen();
};

class Negative : public Expression {
    std::unique_ptr<Expression>  expression;
public:
    virtual NodeType getNodeType() const { return NodeType::NEGATIVE; }
    Negative(std::unique_ptr<Expression> Expression) : expression(std::move(Expression)) {}
    virtual llvm::Value *codegen();
};

class Variable : public Expression{
public:
    virtual NodeType getNodeType() const { return NodeType::VARIABLE; }
    Variable(const std::string name) : Name(name){}
    std::string Name;
    virtual llvm::Value *codegen();
    virtual std::pair<llvm::Value*, llvm::Type*> getAddressAndType();
};

class Indexing : public Expression{
    std::unique_ptr<Expression> Index;
    std::unique_ptr<Expression> Object;
public:
    virtual NodeType getNodeType() const { return NodeType::INDEXING; }
    Indexing(std::unique_ptr<Expression> object, std::unique_ptr<Expression> index) :
            Object(move(object)), Index(move(index)) {}
    virtual llvm::Value *codegen();
    virtual std::pair<llvm::Value*, llvm::Type*> getAddressAndType();
};

class BinaryExpression : public Expression{
    std::string Op;
    std::unique_ptr<Expression> LHS, RHS;
public:
    virtual NodeType getNodeType() const { return NodeType::BINARY_EXPRESSION; }
    BinaryExpression(std::string op, std::unique_ptr<Expression> lhs,
                            std::unique_ptr<Expression> rhs):
                            Op(op), LHS(move(lhs)), RHS(move(rhs)) {}
    virtual llvm::Value *codegen();
};

class Call : public Expression{
    std::string Callee;
    std::vector<std::unique_ptr<Expression>> Arguments;
public:
    virtual NodeType getNodeType() const { return NodeType::CALL; }
    Call(const std::string callee, std::vector<std::unique_ptr<Expression>> arguments) :
            Callee(callee), Arguments(move(arguments)){}
    virtual llvm::Value *codegen();
    virtual std::pair<llvm::Value*, llvm::Type*> getAddressAndType();
};

class VariableDefinition : public Statement {
    std::string Name;
    std::unique_ptr<Expression> Value = nullptr;
public:
    virtual NodeType getNodeType() const { return NodeType::VARIABLE_DEFINITION; }
    VariableDefinition(std::string name, std::unique_ptr<Type> type, std::unique_ptr<Expression> Init) :
            Statement(move(type)), Name(name), Value(std::move(Init)) {}
    VariableDefinition(std::string name, std::unique_ptr<Type> type) : Statement(move(type)), Name(name) {}
    virtual llvm::Value *codegen();
};

class IfStatement : public Statement {
    std::unique_ptr<Expression> Condition;
    std::vector<std::unique_ptr<Node>> Then, Else;
public:
    virtual NodeType getNodeType() const { return NodeType::IF_STATEMENT; }
    IfStatement(std::unique_ptr<Expression> Cond, std::vector<std::unique_ptr<Node>> Then, std::vector<std::unique_ptr<Node>> Else) :
        Condition(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {};
    virtual llvm::Value *codegen();
};

class ForLoop : public Node {
    std::string VariableName;
    std::unique_ptr<Expression> Start, Condition, Step;
    std::vector<std::unique_ptr<Node>> Body;
public:
    virtual NodeType getNodeType() const { return NodeType::FOR_LOOP; }
    ForLoop(std::string VariableName, std::unique_ptr<Expression> Start, std::unique_ptr<Expression>Condition,
            std::unique_ptr<Expression> Step, std::vector<std::unique_ptr<Node>> Body) : VariableName(VariableName),
                                                                                   Start(std::move(Start)), Condition(std::move(Condition)), Step(std::move(Step)),
                                                                                   Body(std::move(Body)) {};
    virtual llvm::Value *codegen();
};

class WhileLoop : public Statement {
    std::unique_ptr<Node> Condition;
    std::vector<std::unique_ptr<Node>> Body;
public:
    virtual NodeType getNodeType() const { return NodeType::WHILE_LOOP; }
    WhileLoop(std::unique_ptr<Node> Condition, std::vector<std::unique_ptr<Node>> Body) : Condition(std::move(Condition)),
                                                                                   Body(std::move(Body)) {};
    virtual llvm::Value *codegen();
};

class Return : public Statement {
    std::unique_ptr<Node> Expression;
public:
    virtual NodeType getNodeType() const { return NodeType::RETURN; }
    Return(std::unique_ptr<Node> Expression) :
    Expression(std::move(Expression)) {};
    virtual llvm::Value *codegen();
};

class Function : public Statement{
    std::string Name;
    std::vector<std::pair<std::unique_ptr<Type>, std::string>> Arguments;
    std::vector<std::unique_ptr<Node>> Body;
public:
    virtual NodeType getNodeType() const { return NodeType::FUNCTION; }
    Function(const std::string name, std::unique_ptr<Type> type,
             std::vector<std::pair<std::unique_ptr<Type>, std::string>> arguments,
             std::unique_ptr<Node> body) :
            Name(name), Statement(move(type)), Arguments(move(arguments)) {
        Body.push_back(std::move(body));
    };
    Function(const std::string name, std::unique_ptr<Type> type,
             std::vector<std::pair<std::unique_ptr<Type> , std::string>> arguments,
             std::vector<std::unique_ptr<Node>> body) :
            Name(name), Statement(move(type)), Arguments(move(arguments)), Body(move(body)) {}

    virtual llvm::Value *codegen();
};

class Extern : public Statement {
    std::string  Name;
    std::vector<std::pair<std::unique_ptr<Type>, std::string>> Arguments;
public:
    virtual NodeType getNodeType() const { return NodeType::EXTERN; }
    Extern(const std::string name, std::unique_ptr<Type> type,
           std::vector<std::pair<std::unique_ptr<Type>, std::string>> arguments) :
           Name(name), Statement(move(type)), Arguments(move(arguments)) {}
    virtual llvm::Value *codegen();
};

#endif //T_NODES_H
