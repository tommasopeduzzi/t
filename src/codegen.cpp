//
// Created by Tommaso Peduzzi on 18.10.21.
//

#include "nodes.h"
#include "error.h"
#include <map>
#include <llvm/IR/IRBuilder.h>
#include "type.h"
#include "symbols.h"

std::unique_ptr<llvm::LLVMContext> Context;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::unique_ptr<llvm::Module> Module;
Symbols Symbols;

void InitializeLLVM() {
    Context = std::make_unique<llvm::LLVMContext>();
    Module = std::make_unique<llvm::Module>("t", *Context);
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
}

llvm::AllocaInst *CreateAlloca(llvm::Function *Function, llvm::Type* Type, const std::string &Name, int Size=1) {
    return Builder->CreateAlloca(Type,llvm::ConstantInt::get(llvm::Type::getDoubleTy(*Context), Size), Name);
}

std::pair<llvm::Value*, llvm::Type*> Variable::getAddressAndType() {
    auto variable = Symbols.GetVariable(Name);
    return std::make_pair(variable.address, variable.type->GetLLVMType());
}

std::pair<llvm::Value*, llvm::Type*> Call::getAddressAndType() {
    auto function = Symbols.GetFunction(Callee);
    return std::make_pair(codegen(), function.type->GetLLVMType());
}

std::pair<llvm::Value*, llvm::Type*> Indexing::getAddressAndType() {
    auto ObjectAddressAndType = Object->getAddressAndType();
    return { Builder->CreateGEP(ObjectAddressAndType.second, ObjectAddressAndType.first, Builder->CreateFPToUI(Index->codegen(), llvm::Type::getInt16Ty(*Context))), ObjectAddressAndType.second};
}

llvm::Value *Negative::codegen(){
    auto *Value = expression->codegen();
    if(!Value){
        return nullptr;
    }
    return Builder->CreateFMul(llvm::ConstantFP::get(*Context,llvm::APFloat(-1.0)), Value, "neg");
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
    auto AddressAndType = getAddressAndType();
    return Builder->CreateLoad(AddressAndType.second, AddressAndType.first);
}

llvm::Value *Indexing::codegen(){
    auto AddressAndType = getAddressAndType();
    return Builder->CreateLoad(AddressAndType.second, AddressAndType.first);
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
        // TODO: Change this to have a different value based on type.
        initialValue = llvm::ConstantFP::get(*Context, llvm::APFloat(0.0));
    }
    auto Alloca = CreateAlloca(Function, type->GetLLVMType(), Name, type->size);
    Symbols.CreateVariable(Name, type, Alloca);
    return Builder->CreateStore(initialValue, Alloca);
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

llvm::Value *BinaryExpression::codegen(){
    if(Op == "="){
        auto AddressAndType = LHS->getAddressAndType();

        auto Value = RHS->codegen();
        if(!Value)
            return nullptr;

        Builder->CreateStore(Value, AddressAndType.first);
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
    auto ExpressionValue = Value->codegen();
    if(!ExpressionValue)
        return nullptr;
    return Builder->CreateRet(ExpressionValue);
}

llvm::Value *IfStatement::codegen() {
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
    Symbols.CreateScope();
    for(auto &Expression : Then){
        auto ExpressionIR = Expression->codegen();

        if(!ExpressionIR)
            return nullptr;
    }
    if(Builder->GetInsertBlock()->getTerminator() == nullptr)
        Builder->CreateBr(After);
    Symbols.DestroyScope();

    Function->getBasicBlockList().push_back(ElseBlock);
    Builder->SetInsertPoint(ElseBlock);
    Symbols.CreateScope();
    for(auto &Expression : Else){
        auto ExpressionIR = Expression->codegen();

        if(!ExpressionIR)
            return nullptr;
    }
    if(Builder->GetInsertBlock()->getTerminator() == nullptr)
        Builder->CreateBr(After);
    Symbols.DestroyScope();

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

    Symbols.CreateScope();
    auto Alloca = CreateAlloca(Function, llvm::Type::getDoubleTy(*Context), VariableName);
    Symbols.CreateVariable(VariableName, std::make_shared<Type>("number"), Alloca);
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
    Symbols.DestroyScope();
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
    Symbols.CreateScope();

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
    Symbols.DestroyScope();

    Builder->SetInsertPoint(AfterBlock);

    return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*Context));
}

llvm::Value *Function::codegen() {
    llvm::Function *Function = Module->getFunction(Name);
    if(!Function){
        // Create Vector that specifies the types for the arguments (atm only floating point numbers aka doubles)
        std::vector<llvm::Type*> ArgumentTypes (Arguments.size());
        for (int i = 0; i < Arguments.size();i++){
            ArgumentTypes[i] = Arguments[i].first->GetLLVMType();
        }
        llvm::FunctionType *FunctionType = llvm::FunctionType::get(type->GetLLVMType(), ArgumentTypes, false);
        Function = llvm::Function::Create(FunctionType, llvm::Function::ExternalLinkage, Name, Module.get());
        int i = 0;
        for (auto &Argument : Function->args()) {
            Argument.setName(Arguments[i].second);
            i += 1;
        }
    }

    if(!Function->empty())
        return LogError("Can't redefine Function");

    //Define BasicBlock to start inserting into for function
    llvm::BasicBlock *BasicBlock = llvm::BasicBlock::Create(*Context, "entry", Function);
    Builder->SetInsertPoint(BasicBlock);

    Symbols.CreateScope();
    int argument = 0;
    for(auto &Arg : Function->args()){
        Arg.setName(Arguments[argument].second);
        llvm::AllocaInst *Alloca = CreateAlloca(Function, Arguments[argument].first->GetLLVMType(), Arg.getName().str());
        Symbols.CreateVariable(Arg.getName().str(), Arguments[argument].first, Alloca);
        Builder->CreateStore(&Arg, Alloca);
        argument += 1;
    }
    Symbols.CreateFunction(Name, type, Arguments, Function);
    for(int i = 0; i < Body.size(); i++){
        auto value = Body[i]->codegen();

        if(!value){
            Function->eraseFromParent();    // error occurred delete the function
            return Function;
        }
    }
    Symbols.DestroyScope();
    return Function;
}

llvm::Value *Extern::codegen() {
    llvm::Function *Function = Module->getFunction(Name);
    if(!Function){
        std::vector<llvm::Type*> ArgumentTypes (Arguments.size(), llvm::Type::getDoubleTy(*Context));
        for (int i = 0; i < Arguments.size();i++){
            ArgumentTypes[i] = Arguments[i].first->GetLLVMType();
        }
        llvm::FunctionType *FunctionType = llvm::FunctionType::get(type->GetLLVMType(), ArgumentTypes, false);
        Function = llvm::Function::Create(FunctionType, llvm::Function::ExternalLinkage, Name, Module.get());
        int i = 0;
        for (auto &Argument : Function->args()) {
            Argument.setName(Arguments[i].second);
            i += 1;
        }
    }
    Symbols.CreateFunction(Name, type, Arguments, Function);
    return Function;
}