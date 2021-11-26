//
// Created by Tommaso Peduzzi on 18.10.21.
//

#include "nodes.h"
#include "error.h"
#include <map>
#include <llvm/IR/IRBuilder.h>

std::unique_ptr<llvm::LLVMContext> Context;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::unique_ptr<llvm::Module> Module;
static std::vector<std::map<std::string, std::pair<llvm::Type *, llvm::AllocaInst *>>> Variables;

void InitializeLLVM() {
    Context = std::make_unique<llvm::LLVMContext>();
    Module = std::make_unique<llvm::Module>("t", *Context);
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
}

llvm::AllocaInst *CreateAlloca(llvm::Function *Function,
                               const std::string &Name){
    llvm::IRBuilder<> TempBuilder(&Function->getEntryBlock(),
                                  Function->getEntryBlock().begin());
    return TempBuilder.CreateAlloca(llvm::Type::getDoubleTy(*Context),0,
                                 Name.c_str());
}

llvm::Type *GetType(std::string Name){
    if (Name == "number")
        return llvm::Type::getDoubleTy(*Context);
    else if (Name == "bool")
        return llvm::Type::getInt1Ty(*Context);
    else if (Name == "void")
        return llvm::Type::getVoidTy(*Context);
    else
        return llvm::Type::getInt8PtrTy(*Context);
}

void CreateScope(){
    Variables.push_back(std::map<std::string, std::pair<llvm::Type *, llvm::AllocaInst *>>());
}

void DestroyScope(){
    Variables.pop_back();
}

void CreateVariable(std::string Name, llvm::AllocaInst *Alloca, llvm::Type *Type){
    Variables.back()[Name] = std::make_pair(Type, Alloca);
}

std::pair<llvm::Type*, llvm::AllocaInst*> GetVariable(std::string name){
    for(auto &Scope : Variables){
        if(Scope.find(name) != Scope.end()){
            return Scope[name];
        }
    }
    return std::make_pair(nullptr, nullptr);
}

llvm::Value *Negative::codegen(){
    auto *Value = Expression->codegen();
    if(!Value){
        return nullptr;
    }
    return Builder->CreateFMul(llvm::ConstantFP::get(*Context,
                                                     llvm::APFloat(-1.0)),
                               Value, "neg");
}

llvm::Value *Number::codegen() {
    return llvm::ConstantFP::get(*Context, llvm::APFloat(Value));
}

llvm::Value *Bool::codegen() {
    if(Value)
        return llvm::ConstantInt::getTrue(*Context);
    else
        return llvm::ConstantInt::getFalse(*Context);
}

llvm::Value *String::codegen(){
    return Builder->CreateGlobalStringPtr(llvm::StringRef(Value));
}

llvm::Value *Variable::codegen(){
    auto Variable = GetVariable(Name);
    if(!Variable.first || !Variable.second)
        return LogError("Unknown Variable");
    return Builder->CreateLoad(Variable.first, Variable.second);
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
    auto Type = GetType(this->Type);
    auto Alloca = CreateAlloca(Function, Name);
    CreateVariable(Name, Alloca, Type);
    return Builder->CreateStore(initialValue, Alloca);
}

llvm::Value *BinaryExpression::codegen(){
    if(Op == "="){
        auto L = static_cast<Variable *>(LHS.get());
        if(!L)
            return LogError("Expected variable.");

        auto Value = RHS->codegen();
        if(!Value)
            return nullptr;

        auto Variable = GetVariable(L->Name);
        if(!Variable.first || !Variable.second)
            return LogError("Unknown Variable");

        Builder->CreateStore(Value, Variable.second);
        return Value;
    }

    auto L = LHS->codegen();
    auto R = RHS->codegen();

    if(!L || !R)
        return LogError("Error Parsing BinaryExpression");
    if(Op == "+")
        return Builder->CreateFAdd(L, R);
    else if(Op == "-")
        return Builder->CreateFSub(L, R);
    else if (Op == "*")
        return Builder->CreateFMul(L, R);
    else if (Op == "/")
        return Builder->CreateFDiv(L, R);
    else if (Op == "<")
        return Builder->CreateFCmpULT(L,R);
    else if (Op == ">")
        return Builder->CreateFCmpUGT(L,R);
    else if (Op == ">=")
        return Builder->CreateFCmpUGE(L,R);
    else if (Op == "<=")
        return Builder->CreateFCmpULE(L,R);
    else if (Op == "==")
        return Builder->CreateFCmpOEQ(L,R);
    else
        return LogError("Unrecognized Operator.");
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
    if(ConditionValue->getType() != llvm::Type::getInt1Ty(*Context))
        ConditionValue = Builder->CreateFCmpONE(ConditionValue,
                                                llvm::ConstantFP::get(*Context, llvm::APFloat(0.0)));

    auto Function = Builder->GetInsertBlock()->getParent();

    auto ThenBlock = llvm::BasicBlock::Create(*Context, "then", Function);
    auto ElseBlock = llvm::BasicBlock::Create(*Context, "else");
    auto After = llvm::BasicBlock::Create(*Context, "continue");

    llvm::BranchInst* conditionInstruction;
    conditionInstruction = Builder->CreateCondBr(ConditionValue, ThenBlock, ElseBlock);

    Builder->SetInsertPoint(ThenBlock);
    CreateScope();
    for(auto &Expression : Then){
        auto ExpressionIR = Expression->codegen();

        if(!ExpressionIR)
            return nullptr;
    }
    if(Builder->GetInsertBlock()->getTerminator() == nullptr)
        Builder->CreateBr(After);
    DestroyScope();

    Function->getBasicBlockList().push_back(ElseBlock);
    Builder->SetInsertPoint(ElseBlock);
    CreateScope();
    for(auto &Expression : Else){
        auto ExpressionIR = Expression->codegen();

        if(!ExpressionIR)
            return nullptr;
    }
    if(Builder->GetInsertBlock()->getTerminator() == nullptr)
        Builder->CreateBr(After);
    DestroyScope();

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
    CreateVariable(VariableName, Alloca, llvm::Type::getDoubleTy(*Context));
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

    if(EndCondition->getType() != llvm::Type::getInt1Ty(*Context))
        EndCondition = Builder->CreateFCmpONE(
            EndCondition, llvm::ConstantFP::get(*Context, llvm::APFloat(0.0)), "conditon");

    auto AfterBlock = llvm::BasicBlock::Create(*Context, "afterloop", Function);
    Builder->CreateCondBr(EndCondition, ForLoopBlock, AfterBlock);
    DestroyScope();
    Builder->SetInsertPoint(AfterBlock);
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

    if(ConditionValue->getType() != llvm::Type::getInt1Ty(*Context))
        ConditionValue = Builder->CreateFCmpONE(ConditionValue, llvm::ConstantFP::get(*Context, llvm::APFloat(0.0)), "condition");

    Builder->CreateCondBr(ConditionValue, WhileLoopBlock, AfterBlock);
    DestroyScope();

    Builder->SetInsertPoint(AfterBlock);

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
        std::vector<llvm::Type*> ArgumentTypes (Arguments.size());
        for (int i = 0; i < Arguments.size();i++){
            ArgumentTypes[i] = GetType(Arguments[i].first);
        }
        llvm::FunctionType *FunctionType = llvm::FunctionType::get(GetType(Type), ArgumentTypes, false);
        Function = llvm::Function::Create(FunctionType, llvm::Function::ExternalLinkage, Name, Module.get());
        int i = 0;
        for (auto &Argument : Function->args()) {
            auto name = Arguments[i];
            Argument.setName(Arguments[i].second);
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
        auto type = Arg.getType();
        CreateVariable(Arg.getName().str(), Alloca, type);
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
        for (int i = 0; i < Arguments.size();i++){
            ArgumentTypes[i] = GetType(Arguments[i].first);
        }
        llvm::FunctionType *FunctionType = llvm::FunctionType::get(GetType(Type), ArgumentTypes, false);
        Function = llvm::Function::Create(FunctionType, llvm::Function::ExternalLinkage, Name, Module.get());
        int i = 0;
        for (auto &Argument : Function->args()) {
            auto name = Arguments[i];
            Argument.setName(Arguments[i].second);
            i += 1;
        }
    }
    return Function;
}
