//
// Created by Tommaso Peduzzi on 13.01.22.
//

#ifndef T_SYMBOLS_H
#define T_SYMBOLS_H
#include "error.h"

class Symbols {
    struct Variable{
        std::shared_ptr<Type> type;
        llvm::Value* address;
    };
    struct Argument{
        std::shared_ptr<Type> type;
        std::string name;
    };
    struct Function{
        std::shared_ptr<Type> type;
        std::vector<Argument> arguments;
        llvm::Function* function;
    };

    std::vector<std::map<std::string, Variable>> Variables;
    std::map<std::string, Function> Functions;
public:
    void Reset(){
        Variables = std::vector<std::map<std::string, Variable>>();
        Variables.push_back(std::map<std::string, Variable>());
        Functions = std::map<std::string, Function>();
    }

    void CreateVariable(std::string name, std::shared_ptr<Type> type, llvm::Value *value = nullptr){
        Variables.back()[name] = {type, value};
    }

    void CreateScope(){
        Variables.push_back(std::map<std::string, Variable>());
    }

    void DestroyScope(){
        Variables.pop_back();
    }

    void CreateFunction(std::string name, std::shared_ptr<Type> returnType, std::vector<std::pair<std::shared_ptr<Type>, std::string>> arguments, llvm::Function* function = nullptr){
        std::vector<Argument> args;
        for (auto& arg : arguments)
            args.push_back({arg.first, arg.second});
        Functions[name] = {returnType, args, function};
    }

    Variable GetVariable(std::string name){
        for(auto Scope : Variables){
            auto it = Scope.find(name);
            if(it != Scope.end())
                return Scope[name];
        }
        LogError("Variable " + name + " not found");
        exit(1);
    }

    Function GetFunction(std::string name){
        auto it = Functions.find(name);
        if(it != Functions.end())
            return Functions[name];
        LogError("Function " + name + " not found");
        exit(1);
    }

};


#endif //T_SYMBOLS_H
