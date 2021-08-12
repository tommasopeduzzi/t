//
// Created by tommasopeduzzi on 12/08/2021.
//

#ifndef T_NODES_H
#define T_NODES_H

#include <vector>

class Node{
public:
    virtual ~Node() {}
};

class Number : public Node{
    double Value;
public:
    Number(const double value) : Value(value) {}
};

class Variable : public Node{
    std::string Name;

public:
    Variable(const std::string name) : Name(name){}
};

class BinaryExpression : public Node{
    char Op;
    std::unique_ptr<Node> LHS, RHS;

public:
    BinaryExpression(char op, std::unique_ptr<Node> lhs,
                            std::unique_ptr<Node> rhs):
                            Op(op), LHS(move(lhs)), RHS(move(rhs)) {}
};

class Call : public Node{
    std::string Callee;
    std::vector<std::unique_ptr<Node>> Arguments;

public:
    Call(const std::string callee, std::vector<std::unique_ptr<Node>> arguments) :
    Callee(callee), Arguments(move(arguments)){}
};

class Function{
    std::string Name;
    std::vector<std::unique_ptr<Node>> Arguments;
    std::unique_ptr<Node> Body;

public:
    Function(const std::string name,
             std::vector<std::unique_ptr<Node>> arguments,
             std::unique_ptr<Node> body) :
            Name(name), Arguments(move(arguments)), Body(move(body)) {}

    std::string getName(){
        return Name;
    }
};

#endif //T_NODES_H
