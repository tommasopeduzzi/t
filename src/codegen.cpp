//
// Created by Tommaso Peduzzi on 18.10.21.
//

#include "nodes.h"
#include "error.h"
#include "llvm/IR/Verifier.h"
#include <map>
std::unique_ptr<llvm::LLVMContext> Context;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::unique_ptr<llvm::Module> Module;
static llvm::AnalysisManager<llvm::Function> MAM;
static std::unique_ptr<llvm::PassManager<llvm::Function>> FunctionOptimizer;
static std::vector<std::map<std::string, llvm::AllocaInst *>> Variables;

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
    FunctionOptimizer->addPass(llvm::VerifierPass());
}

llvm::AllocaInst *CreateAlloca(llvm::Function *Function,
                               const std::string &Name){
    llvm::IRBuilder<> TempBuilder(&Function->getEntryBlock(),
                                  Function->getEntryBlock().begin());
    return TempBuilder.CreateAlloca(llvm::Type::getDoubleTy(*Context),0,
                                 Name.c_str());
}

void CreateScope(){
    Variables.push_back(std::map<std::string, llvm::AllocaInst *>());
}

void DestroyScope(){
    Variables.pop_back();
}

void CreateVariable(std::string Name, llvm::AllocaInst *Value){
    Variables.back()[Name] = Value;
}

llvm::Value *GetVariable(std::string name){
    for(auto &Scope : Variables){
        if(Scope.find(name) != Scope.end()){
            return Scope[name];
        }
    }
    return nullptr;
}

llvm::Value *Number::codegen() {
    return llvm::ConstantFP::get(*Context, llvm::APFloat(Value));
}

llvm::Value *Variable::codegen(){
    llvm::Value *value = GetVariable(Name);
    if(!value)
        return LogError("Unknown Variable");
    return Builder->CreateLoad(value);
}

llvm::Value *VariableDefinition::codegen(){
    auto Function = Builder->GetInsertBlock()->getParent();

    llvm::Value *initialValue;
    if(Value){
        initialValue = Value->codegen();
        if(!initialValue)
            return nullptr;
    }
    else{
        initialValue = llvm::ConstantFP::get(*Context, llvm::APFloat(0.0));
    }

    auto Alloca = CreateAlloca(Function, Name);
    CreateVariable(Name, Alloca);
    return Builder->CreateStore(initialValue, Alloca);
}

llvm::Value *BinaryExpression::codegen(){
    if(Op == '='){
        auto L = static_cast<Variable *>(LHS.get());
        if(!L)
            return LogError("Expected variable.");

        auto Value = RHS->codegen();
        if(!Value)
            return nullptr;

        auto Variable = GetVariable(L->Name);
        if(!Variable)
            return LogError("Unknown Variable");

        Builder->CreateStore(Value, Variable);
        return Value;
    }

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

llvm::Value *Return::codegen() {
    auto ExpressionValue = Expression->codegen();
    if(!ExpressionValue)
        return nullptr;
    return Builder->CreateRet(ExpressionValue);
}

llvm::Value *IfExpression::codegen() {
    auto ConditionValue = Condition->codegen();
    if(!ConditionValue)
        return nullptr;

    ConditionValue = Builder->CreateFCmpONE(ConditionValue,
                                         llvm::ConstantFP::get(*Context, llvm::APFloat(0.0)));

    auto Function = Builder->GetInsertBlock()->getParent();

    auto ThenBlock = llvm::BasicBlock::Create(*Context, "then", Function);
    auto ElseBlock = llvm::BasicBlock::Create(*Context, "else");
    auto After = llvm::BasicBlock::Create(*Context, "continue");

    llvm::BranchInst* conditionInstruction;
    conditionInstruction = Builder->CreateCondBr(ConditionValue, ThenBlock, ElseBlock);

    Builder->SetInsertPoint(ThenBlock);
    for(auto &Expression : Then){
        auto ExpressionIR = Expression->codegen();

        if(!ExpressionIR)
            return nullptr;
    }
    Builder->CreateBr(After);

    Function->getBasicBlockList().push_back(ElseBlock);
    Builder->SetInsertPoint(ElseBlock);
    for(auto &Expression : Else){
        auto ExpressionIR = Expression->codegen();

        if(!ExpressionIR)
            return nullptr;
    }
    Builder->CreateBr(After);

    Function->getBasicBlockList().push_back(After);
    Builder->SetInsertPoint(After);

    return conditionInstruction;
}

llvm::Value *ForLoop::codegen(){
    auto Function = Builder->GetInsertBlock()->getParent();
    auto ForLoopBlock = llvm::BasicBlock::Create(*Context, "loop", Function);

    auto StartValue = Start->codegen();
    if(!StartValue)
        return nullptr;

    CreateScope();
    auto Alloca = CreateAlloca(Function, VariableName);
    CreateVariable(VariableName, Alloca);
    Builder->CreateStore(StartValue, Alloca);

    Builder->CreateBr(ForLoopBlock);
    Builder->SetInsertPoint(ForLoopBlock);

    for(auto &Expression : Body){
        auto ExpressionIR = Expression->codegen();
        if(!ExpressionIR)
            return nullptr;
    }

    llvm::Value *StepValue;
    if(Step){
        StepValue = Step->codegen();
        if(!StepValue)
            return nullptr;
    }
    else{
        StepValue = llvm::ConstantFP::get(*Context, llvm::APFloat(1.0));
    }
    auto CurrentStep = Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, VariableName);
    auto NextStep = Builder->CreateFAdd(CurrentStep, StepValue, "step");
    Builder->CreateStore(NextStep, Alloca);

    llvm::Value *EndCondition = Condition->codegen();
    if(!EndCondition)
        return nullptr;

    EndCondition = Builder->CreateFCmpONE(
            EndCondition, llvm::ConstantFP::get(*Context, llvm::APFloat(0.0)), "conditon");

    auto AfterBlock = llvm::BasicBlock::Create(*Context, "afterloop", Function);
    Builder->CreateCondBr(EndCondition, ForLoopBlock, AfterBlock);
    Builder->SetInsertPoint(AfterBlock);
    DestroyScope();
    return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*Context));
}

llvm::Value *WhileLoop::codegen(){
    auto Function = Builder->GetInsertBlock()->getParent();
    auto WhileLoopBlock = llvm::BasicBlock::Create(*Context, "whileloop", Function);
    auto AfterBlock = llvm::BasicBlock::Create(*Context, "afterloop", Function);
    llvm::Value *ConditionValue = Condition->codegen();
    if(!ConditionValue)
        return nullptr;

    ConditionValue = Builder->CreateFCmpONE(ConditionValue, llvm::ConstantFP::get(*Context, llvm::APFloat(0.0)), "condition");

    Builder->CreateCondBr(ConditionValue, WhileLoopBlock, AfterBlock);
    Builder->SetInsertPoint(WhileLoopBlock);
    CreateScope();

    for(auto &Expression : Body){
        auto ExpressionIR = Expression->codegen();
        if(!ExpressionIR)
            return nullptr;
    }

    ConditionValue = Condition->codegen();
    if(!ConditionValue)
        return nullptr;

    ConditionValue = Builder->CreateFCmpONE(ConditionValue, llvm::ConstantFP::get(*Context, llvm::APFloat(0.0)), "condition");

    Builder->CreateCondBr(ConditionValue, WhileLoopBlock, AfterBlock);
    Builder->SetInsertPoint(AfterBlock);
    DestroyScope();

    return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*Context));
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

    CreateScope();
    for(auto &Arg : Function->args()){
        llvm::AllocaInst *Alloca = CreateAlloca(Function, Arg.getName().str());
        Builder->CreateStore(&Arg, Alloca);
        CreateVariable(Arg.getName().str(), Alloca);
    }

    for(int i = 0; i < Body.size(); i++){
        auto value = Body[i]->codegen();

        if(!value){
            Function->eraseFromParent();    // error occurred delete the function
            return Function;
        }
    }
    DestroyScope();
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
