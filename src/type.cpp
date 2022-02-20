//
// Created by Tommaso Peduzzi on 06.01.22.
//

#include <llvm/IR/Type.h>
#include "codegen.h"
#include "type.h"
#include "error.h"
#include "nodes.h"

using namespace std;
using namespace llvm;

namespace t {

    llvm::Type *Type::GetLLVMType() const {
        if (type.empty()) {
            assert(false);
        }
        if (type == "number")
            return llvm::Type::getDoubleTy(*Context);
        else if (type == "string")
            return llvm::Type::getInt8PtrTy(*Context);
        else if (type == "bool")
            return llvm::Type::getInt1Ty(*Context);
        else if (type == "void")
            return llvm::Type::getVoidTy(*Context);
        else{
            auto Structure = Symbols.GetStructure(type);
            if (Structure.members.empty() && Structure.type == nullptr) {
                LogError("Unknown type '" + type + "'");
                exit(1);
            }
            return Structure.type;
        }
    }

    bool operator==(Type &lhs, Type &rhs) {
        return lhs.type == rhs.type && lhs.subtype == rhs.subtype && lhs.size == rhs.size;
    }

    bool operator!=(Type &lhs, Type &rhs) {
        return !(rhs == lhs);
    }

    bool operator==(shared_ptr<Type> lhs, shared_ptr<Type> rhs) {
        if (lhs == nullptr && rhs != nullptr || rhs == nullptr && lhs != nullptr)
            return false;
        return *lhs == *rhs;
    }

    bool operator!=(shared_ptr<Type> lhs, shared_ptr<Type> rhs) {
        return !(rhs == lhs);
    }

    void Number::checkType() {
        type = make_shared<Type>("number", 1);
    }

    void String::checkType() {
        type = make_shared<Type>("string", 1);
    }

    void Bool::checkType() {
        type = make_shared<Type>("boolean", 1);
    }

    void Negative::checkType() {
        // TODO: actually check if the type can be negative
        expression->checkType();
        type = expression->type;
    }

    void Variable::checkType() {
        auto variable = Symbols.GetVariable(Name);
        if (variable.type == nullptr && variable.address == nullptr) {
            LogError(location, "Variable " + Name + " not found!");
            exit(1);
        }
        type = variable.type;
    }

    void Indexing::checkType() {
        Object->checkType();
        Index->checkType();
        if (Index->type != make_shared<Type>("number")) {
            LogError(location, "Index must be a number");
            exit(1);
        }
        type = make_shared<Type>(*(Object->type));
        type->size = 1;
    }

    void Call::checkType() {
        auto function = Symbols.GetFunction(Callee);
        if (function.type == nullptr && function.function == nullptr) {
            LogError(location, "Function " + Callee + " not found!");
        }
        auto arguments = function.arguments;
        for (int i = 0; i < Arguments.size(); i++) {
            Arguments[i]->checkType();
            if (arguments[i].type != Arguments[i]->type) {
                LogError(location, "Wrong type of argument");
                exit(1);
            }
        }
        type = function.type;
    }

    void BinaryExpression::checkType() {
        LHS->checkType();
        RHS->checkType();

        if (LHS->type != RHS->type) {
            LogError(location, "Type mismatch");
            exit(1);
        }

        type = LHS->type; // TODO: fix this mess
    }

    void VariableDefinition::checkType() {
        Symbols.CreateVariable(Name, type);

        if (Value) {
            Value->checkType();
            if (Value->type != type) {
                LogError(location, "Value Type and Variable Type mismatch");
                exit(1);
            }
        }
    }

    void IfStatement::checkType() {
        Condition->checkType(); // TODO: check if condition is boolean

        Symbols.CreateScope();
        for (auto &node: Then) {
            node->checkType();
        }
        Symbols.DestroyScope();
        Symbols.CreateScope();
        for (auto &node: Else) {
            node->checkType();
        }
        Symbols.DestroyScope();
        type = make_shared<Type>("void");
    }

    void ForLoop::checkType() {
        Start->checkType();
        if (Start->type != make_shared<Type>("number")) {
            LogError(location, "Start-Value for Stepper in For-Loop must be a number");
            exit(1);
        }
        Symbols.CreateScope();
        Symbols.CreateVariable(VariableName, make_shared<Type>("number"));
        Condition->checkType(); // TODO: check if condition is boolean
        Step->checkType();
        if (Step->type != make_shared<Type>("number")) {
            LogError(location, "Step-Value for stepper must be a number");
            exit(1);
        }
        //TODO: make start and step type changeable and check if the type of the step is compatible with the type of the start (they have to be equal)

        for (auto &node: Body) {
            node->checkType();
        }
        Symbols.DestroyScope();
        type = make_shared<Type>("void");
    }

    void WhileLoop::checkType() {
        Condition->checkType(); // TODO: check if condition is boolean

        Symbols.CreateScope();
        for (auto &node: Body) {
            node->checkType();
        }
        Symbols.DestroyScope();

        type = make_shared<Type>("void");
    }

    void Return::checkType() {
        Value->checkType();
        type = make_shared<Type>("void");
    }

    void Function::checkType() {
        Symbols.CreateFunction(Name, type, Arguments);
        Symbols.CreateScope();
        for (auto &arg: Arguments) {
            Symbols.CreateVariable(arg.second, arg.first);
        }
        for (auto &node: Body) {
            node->checkType();
        }
        Symbols.DestroyScope();
        // type = make_shared<Type>("void"); // TODO: make the type of this node irrelevant in codegenning, so we can correctly save the type of this node
    }

    void Extern::checkType() {
        Symbols.CreateFunction(Name, type, Arguments);
        // type = make_shared<Type>("void"); // TODO: make the type of this node irrelevant in codegenning, so we can correctly save the type of this node
    }

    void Assembly::checkType() {
        type = make_shared<Type>("void");
    }

    void Structure::checkType() {
        type = make_shared<Type>("void");
        Symbols.CreateStructure(Name, Members, nullptr);
    }

    void Member::checkType() {
        Object->checkType();
        auto Structure = Symbols.GetStructure(Object->type->type);
        if (Structure.members == vector<pair<string, shared_ptr<Type>>>() &&  Structure.type == nullptr) {
            LogError(location, "Object is not a structure");
            exit(1);
        }
        for (auto& Member : Structure.members){
            if (Member.first == Name) {
                type = move(Member.second);
                return;
            }
        }
        LogError(location, "Member not found");
        exit(1);
    }
}