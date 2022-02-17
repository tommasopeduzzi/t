//
// Created by tommasopeduzzi on 12/08/2021.
//

#pragma once

#include "type.h"
#include "lexer.h"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <llvm/IR/Value.h>

using namespace std;

namespace t {

    enum NodeType {
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
        EXTERN,
        ASSEMBLY,
        STRUCTURE
    };

    class Node {
    public:
        shared_ptr<Type> type;

        FileLocation location;

        virtual ~Node() = default;

        Node() = default;

        Node(shared_ptr<Type> type = nullptr, FileLocation location = {}) : type(move(type)), location(location){}

        virtual NodeType getNodeType() const { return NodeType::UNKNOWN; }

        virtual llvm::Value *codegen() = 0;

        virtual void checkType() = 0;
    };

    class Expression : public Node {
    public:
        virtual NodeType getNodeType() const { return NodeType::EXPRESSION; }

        Expression() = default;

        Expression(FileLocation location) : Node(nullptr, location){}

        Expression(shared_ptr<Type> type, FileLocation location) : Node(move(type), location) {}

        virtual llvm::Value *codegen() = 0;

        virtual pair<llvm::Value *, llvm::Type *> getAddressAndType() {
            assert(false && "SOMETHING WENT TERRIBLY WRONG");
        }
    };

    class Statement : public Node {
    public:
        virtual NodeType getNodeType() const { return NodeType::STATEMENT; }

        Statement() = default;

        Statement(FileLocation location) : Node(nullptr, location){}

        Statement(shared_ptr<Type> type = nullptr, FileLocation location = {}) : Node(move(type), location) {}

        virtual llvm::Value *codegen() = 0;
    };

// TODO: maybe have another subclass for literals?

    class Number : public Expression {
        double Value;
    public:
        virtual NodeType getNodeType() const { return NodeType::NUMBER; }

        Number(const double value, FileLocation location) : Expression(location), Value(value) {}

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class Bool : public Expression {
        bool Value;
    public:
        virtual NodeType getNodeType() const { return NodeType::BOOL; }

        Bool(const bool value, FileLocation location) : Expression(location), Value(value) {}

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class String : public Expression {
        string Value;
    public:
        virtual NodeType getNodeType() const { return NodeType::STRING; }

        String(string value, FileLocation location) : Expression(location), Value(value) {}

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class Negative : public Expression {
        unique_ptr<Expression> expression;
    public:
        virtual NodeType getNodeType() const { return NodeType::NEGATIVE; }

        Negative(unique_ptr<Expression> expression, FileLocation location) : Expression(location), expression(move(expression)) {}

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class Variable : public Expression {
    public:
        virtual NodeType getNodeType() const { return NodeType::VARIABLE; }

        Variable(const string name, FileLocation location) : Expression(location), Name(name) {}

        string Name;

        virtual llvm::Value *codegen();

        virtual void checkType();

        virtual pair<llvm::Value *, llvm::Type *> getAddressAndType();
    };

    class Indexing : public Expression {
        unique_ptr<Expression> Index;
        unique_ptr<Expression> Object;
    public:
        virtual NodeType getNodeType() const { return NodeType::INDEXING; }

        Indexing(unique_ptr<Expression> object, unique_ptr<Expression> index, FileLocation location) :
            Expression(location), Object(move(object)), Index(move(index)) {}

        virtual llvm::Value *codegen();

        virtual void checkType();

        virtual pair<llvm::Value *, llvm::Type *> getAddressAndType();
    };

    class BinaryExpression : public Expression {
        string Op;
        unique_ptr<Expression> LHS, RHS;
    public:
        virtual NodeType getNodeType() const { return NodeType::BINARY_EXPRESSION; }

        BinaryExpression(string op, unique_ptr<Expression> lhs, unique_ptr<Expression> rhs, FileLocation location) :
                         Expression(location), Op(op), LHS(move(lhs)), RHS(move(rhs)) {}

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class Call : public Expression {
        string Callee;
        std::vector<std::unique_ptr<Expression>> Arguments;
    public:
        virtual NodeType getNodeType() const { return NodeType::CALL; }

        Call(const std::string callee, std::vector<std::unique_ptr<Expression>> arguments, FileLocation location) :
            Expression(location), Callee(callee), Arguments(move(arguments)) {}

        virtual llvm::Value *codegen();

        virtual void checkType();

        virtual std::pair<llvm::Value *, llvm::Type *> getAddressAndType();
    };

    class VariableDefinition : public Statement {
        std::string Name;
        std::unique_ptr<Expression> Value = nullptr;
    public:
        virtual NodeType getNodeType() const { return NodeType::VARIABLE_DEFINITION; }

        VariableDefinition(std::string name, std::unique_ptr<Expression> Init, std::unique_ptr<Type> type, FileLocation location) :
                Statement(move(type), location), Name(name), Value(std::move(Init)) {}

        VariableDefinition(std::string name, std::unique_ptr<Type> type) : Statement(move(type)), Name(name) {}

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class IfStatement : public Statement {
        std::unique_ptr<Expression> Condition;
        std::vector<std::unique_ptr<Node>> Then, Else;
    public:
        virtual NodeType getNodeType() const { return NodeType::IF_STATEMENT; }

        IfStatement(std::unique_ptr<Expression> Cond, std::vector<std::unique_ptr<Node>> Then,
                    std::vector<std::unique_ptr<Node>> Else, FileLocation location) :
                    Statement(location), Condition(move(Cond)), Then(move(Then)), Else(move(Else)) {};

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class ForLoop : public Statement {
        std::string VariableName;
        std::unique_ptr<Expression> Start, Condition, Step;
        std::vector<std::unique_ptr<Node>> Body;
    public:
        virtual NodeType getNodeType() const { return NodeType::FOR_LOOP; }

        ForLoop(std::string VariableName, std::unique_ptr<Expression> Start, std::unique_ptr<Expression> Condition,
                std::unique_ptr<Expression> Step, std::vector<std::unique_ptr<Node>> Body, FileLocation location) :
                    Statement(location), VariableName(VariableName), Start(std::move(Start)), Condition( std::move(Condition)),
                    Step(std::move(Step)),Body(std::move(Body)) {};

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class WhileLoop : public Statement {
        std::unique_ptr<Node> Condition;
        std::vector<std::unique_ptr<Node>> Body;
    public:
        virtual NodeType getNodeType() const { return NodeType::WHILE_LOOP; }

        WhileLoop(std::unique_ptr<Node> Condition, std::vector<std::unique_ptr<Node>> Body, FileLocation location) :
        Statement(location), Condition(std::move(Condition)),Body(std::move(Body)) {};

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class Return : public Statement {
        std::unique_ptr<Expression> Value;
    public:
        virtual NodeType getNodeType() const { return NodeType::RETURN; }

        Return(std::unique_ptr<Expression> expression, FileLocation location) : Statement(location), Value(std::move(expression)) {};

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class Function : public Statement {
        std::string Name;
        std::vector<std::pair<std::shared_ptr<Type>, std::string>> Arguments;
        std::vector<std::unique_ptr<Node>> Body;
    public:
        virtual NodeType getNodeType() const { return NodeType::FUNCTION; }

        Function(const std::string name, std::shared_ptr<Type> type, FileLocation location,
                 std::vector<std::pair<std::shared_ptr<Type>, std::string>> arguments,
                 std::unique_ptr<Node> body) :
                Name(name), Statement(move(type), location), Arguments(move(arguments)) {
            Body.push_back(std::move(body));
        };

        Function(const std::string name, std::shared_ptr<Type> type, FileLocation location,
                 std::vector<std::pair<std::shared_ptr<Type>, std::string>> arguments,
                 std::vector<std::unique_ptr<Node>> body) :
                Name(name), Statement(move(type), location), Arguments(move(arguments)), Body(move(body)) {}

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class Extern : public Statement {
        std::string Name;
        std::vector<std::pair<std::shared_ptr<Type>, std::string>> Arguments;
    public:
        virtual NodeType getNodeType() const { return NodeType::EXTERN; }

        Extern(const std::string name, std::unique_ptr<Type> type, FileLocation location,
               std::vector<std::pair<std::shared_ptr<Type>, std::string>> arguments) :
                Name(name), Statement(move(type), location), Arguments(move(arguments)) {}

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class Assembly : public Statement {
        std::string AssemblyCode;
    public:
        virtual NodeType getNodeType() const { return NodeType::ASSEMBLY; }

        Assembly(const std::string assemblyCode, FileLocation location) :
                Statement(location), AssemblyCode(assemblyCode) {}

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

    class Structure : public Statement {
    public:
        std::string Name;
        map<string, shared_ptr<Type>> Members;

        virtual NodeType getNodeType() const { return NodeType::STRUCTURE; }

        Structure(string Name, map<string, shared_ptr<Type>> members, FileLocation location) :
                Statement(location), Members(move(members)), Name(Name) {}

        virtual llvm::Value *codegen();

        virtual void checkType();
    };

}