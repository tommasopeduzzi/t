//
// Created by Tommaso Peduzzi on 18.10.21.
//

#include "nodes.h"
#include "error.h"
#include <map>
std::unique_ptr<llvm::LLVMContext> Context;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::unique_ptr<llvm::Module> Module;
static std::unique_ptr<llvm::PassManager<llvm::Function>> FunctionOptimizer;
static std::map<std::string, llvm::AllocaInst *> Variables;

void InitializeModule(){
    // Open a new module.
    Module = std::make_unique<llvm::Module>("t", *Context);
}

void InitializeLLVM() {
    // TODO: Make my own JIT, see tutorial @ llvm.org
    Context = std::make_unique<llvm::LLVMContext>();
    InitializeModule();
    // Create a new builder for the module.
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    // Create a new pass manager attached to it.
    FunctionOptimizer = std::make_unique<llvm::FunctionPassManager>();

    /* TODO: Figure out how the new PassSystem works / figure
     out where tf to get the passes from, don't want to write my own.
     Also figure out if I really want to actually use the new PassManager,
     as I read that I can't use Platform-dependant backend.*/

}

llvm::AllocaInst *CreateAlloca(llvm::Function *Function,
                               const std::string &Name){
    llvm::IRBuilder<> TempBuilder(&Function->getEntryBlock(),
                                  Function->getEntryBlock().begin());
    return TempBuilder.CreateAlloca(llvm::Type::getDoubleTy(*Context),0,
                                 Name.c_str());
}

llvm::Value *Number::codegen() {
    return llvm::ConstantFP::get(*Context, llvm::APFloat(Value));
}

llvm::Value *Variable::codegen(){
    llvm::Value *value = Variables[Name];
    if(!value)
        return LogError("Unknown Variable");
    return Builder->CreateLoad(value);
}

llvm::Value *BinaryExpression::codegen(){
    auto L = LHS->codegen();
    auto R = RHS->codegen();

    if(!L || !R)
        return LogError("Error Parsing BinaryExpression");
    switch(Op){
        case '+':
            return Builder->CreateFAdd(L, R);
        case '-':
            return Builder->CreateFSub(L, R);
        case '*':
            return Builder->CreateFMul(L, R);
        case '/':
            return Builder->CreateFDiv(L, R);
        case '<':
            L = Builder->CreateFCmpULT(L,R);
            return Builder->CreateUIToFP(L,  // Convert from integer to float
                                        llvm::Type::getDoubleTy(*Context));
        case '>':
            L = Builder->CreateFCmpUGT(L,R);
            return Builder->CreateUIToFP(L,  // Convert from integer to float
                                        llvm::Type::getDoubleTy(*Context));
        default:
            return LogError("Unrecognized Operator.");
    }
}

llvm::Value *Call::codegen() {
    llvm::Function *function = Module->getFunction(Callee);
    if(!function)
        return LogError("Function not defined!");
    if(function->arg_size() != Arguments.size())
        return LogError(
                "Number of Arguments given does not match the number of arguments of the function.");
    std::vector<llvm::Value*> ArgumentValues = {};
    for(int i = 0; i < Arguments.size(); i++){
        auto value = Arguments[i]->codegen();
        if (!value)
            return nullptr;
        ArgumentValues.push_back(value);
    }
    return Builder->CreateCall(function, ArgumentValues);
}

llvm::Value *Function::codegen() {
    llvm::Function *Function = Module->getFunction(Name);
    if(!Function){
        // Create Vector that specifies the types for the arguments (atm only floating point numbers aka doubles)
        std::vector<llvm::Type*> ArgumentTypes (Arguments.size(), llvm::Type::getDoubleTy(*Context));
        llvm::FunctionType *FunctionType = llvm::FunctionType::get(llvm::Type::getDoubleTy(*Context), ArgumentTypes, false);
        Function = llvm::Function::Create(FunctionType, llvm::Function::ExternalLinkage, Name, Module.get());
        int i = 0;
        for (auto &Argument : Function->args()) {
            auto name = Arguments[i];
            Argument.setName(Arguments[i]);
            i += 1;
        }
    }

    if(!Function->empty())
        return LogError("Can't redefine Function");

    //Define BasicBlock to start inserting into for function
    llvm::BasicBlock *BasicBlock = llvm::BasicBlock::Create(*Context, "entry", Function);
    Builder->SetInsertPoint(BasicBlock);

    Variables.clear();
    for(auto &Arg : Function->args()){
        llvm::AllocaInst *Alloca = CreateAlloca(Function, Arg.getName().str());
        Builder->CreateStore(&Arg, Alloca);
        Variables[std::string(Arg.getName())] = Alloca;
    }

    for(int i = 0; i < Body.size(); i++){
        auto value = Body[i]->codegen();

        if(!value){
            Function->eraseFromParent();    // error occurred delete the function
            return Function;
        }

        if(Body[i]->returnValue || Body.size() == 1)
            Builder->CreateRet(value);
    }
    return Function;
}

llvm::Value *Extern::codegen() {
    llvm::Function *Function = Module->getFunction(Name);
    if(!Function){
        // Create Vector that specifies the types for the arguments (atm only floating point numbers aka doubles)
        std::vector<llvm::Type*> ArgumentTypes (Arguments.size(), llvm::Type::getDoubleTy(*Context));
        llvm::FunctionType *FunctionType = llvm::FunctionType::get(llvm::Type::getDoubleTy(*Context), ArgumentTypes, false);
        Function = llvm::Function::Create(FunctionType, llvm::Function::ExternalLinkage, Name, Module.get());
        int i = 0;
        for (auto &Argument : Function->args()) {
            auto name = Arguments[i];
            Argument.setName(Arguments[i]);
            i += 1;
        }
    }
    return Function;
}
