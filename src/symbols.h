//
// Created by Tommaso Peduzzi on 13.01.22.
//

#pragma once

#include "error.h"

namespace t {

    class Symbols {
        struct Variable {
            shared_ptr<Type> type;
            llvm::Value *address;
        };
        struct Argument {
            shared_ptr<Type> type;
            string name;
        };
        struct Function {
            shared_ptr<Type> type;
            vector<Argument> arguments;
            llvm::Function *function;
        };

        vector<map<string, Variable>> Variables;
        map<string, Function> Functions;
    public:
        void Reset() {
            Variables = vector<map<string, Variable>>();
            Variables.push_back(map<string, Variable>());
            Functions = map<string, Function>();
        }

        void CreateVariable(string name, shared_ptr<Type> type, llvm::Value *value = nullptr) {
            Variables.back()[name] = {type, value};
        }

        void CreateScope() {
            Variables.push_back(map<string, Variable>());
        }

        void DestroyScope() {
            Variables.pop_back();
        }

        void CreateFunction(string name, shared_ptr<Type> returnType, vector<pair<shared_ptr<Type>, string>> arguments,
                            llvm::Function *function = nullptr) {
            vector<Argument> args;
            for (auto &arg: arguments)
                args.push_back({arg.first, arg.second});
            Functions[name] = {returnType, args, function};
        }

        Variable GetVariable(string name) {
            for (auto Scope: Variables) {
                auto it = Scope.find(name);
                if (it != Scope.end())
                    return Scope[name];
            }
            return {nullptr, nullptr};
        }

        Function GetFunction(std::string name) {
            auto it = Functions.find(name);
            if (it != Functions.end())
                return Functions[name];
            return {nullptr, {}, nullptr};
        }

    };
}