//
// Created by Tommaso Peduzzi on 18.10.21.
//

#include "nodes.h"
#include "error.h"
#include <map>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include "type.h"
#include "symbols.h"

using namespace std;
using namespace llvm;

namespace t {

    unique_ptr<LLVMContext> Context;
    unique_ptr<IRBuilder<>> Builder;
    unique_ptr<Module> Module;
    Symbols Symbols;

    void InitializeLLVM() {
        Context = make_unique<LLVMContext>();
        Module = make_unique<llvm::Module>("t", *Context);
        Builder = make_unique<IRBuilder<>>(*Context);
    }

    AllocaInst *CreateAlloca(llvm::Function *Function, llvm::Type *Type, const string Name = "", int Size = 1) {
        return Builder->CreateAlloca(Type, ConstantInt::get(llvm::Type::getDoubleTy(*Context), Size), Name);
    }

    pair<Value *, llvm::Type *> Variable::getAddressAndType() {
        auto variable = Symbols.GetVariable(Name);
        return make_pair(variable.address, variable.type->GetLLVMType());
    }

    pair<Value *, llvm::Type *> Call::getAddressAndType() {
        auto function = Symbols.GetFunction(Callee);
        return make_pair(codegen(), function.type->GetLLVMType());
    }

    pair<Value *, llvm::Type *> Indexing::getAddressAndType() {
        auto ObjectAddressAndType = Object->getAddressAndType();
        if (!Object->type->isDynamicallyIndexable()){
            auto index = Builder->CreateFPToUI(Index->codegen(), llvm::Type::getInt16Ty(*Context));
            auto Address = Builder->CreateGEP(ObjectAddressAndType.second, ObjectAddressAndType.first, index);
            return {Address, ObjectAddressAndType.second};
        }
        else if (Object->type->type == "string"){
            auto index = Builder->CreateFPToUI(Index->codegen(), llvm::Type::getInt32Ty(*Context));
            auto Alloca = CreateAlloca(Builder->GetInsertBlock()->getParent(), llvm::Type::getInt8Ty(*Context), "", 2);
            auto StringAddress = Builder->CreateLoad(llvm::Type::getInt8PtrTy(*Context), ObjectAddressAndType.first);
            auto Address = Builder->CreateGEP(llvm::Type::getInt8Ty(*Context), StringAddress, index);
            Builder->CreateMemCpyInline(Alloca, MaybeAlign(), Address, MaybeAlign(), ConstantInt::get(llvm::Type::getInt16Ty(*Context), 1), false);
            auto NullTerminatorAddress = Builder->CreateGEP(Alloca, ConstantInt::get(llvm::Type::getInt32Ty(*Context), 1));
            Builder->CreateStore(ConstantInt::get(llvm::Type::getInt8Ty(*Context), 0), NullTerminatorAddress);
            auto AddressOfAlloca = CreateAlloca(Builder->GetInsertBlock()->getParent(), llvm::Type::getInt8PtrTy(*Context));
            Builder->CreateStore(Alloca, AddressOfAlloca);
            return {AddressOfAlloca, ObjectAddressAndType.second};
        }

        auto Function = Builder->GetInsertBlock()->getParent();

        auto SizeAddress = Builder->CreateGEP(ObjectAddressAndType.first, {
            ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0),
            ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0), // Pointer to size
        });
        auto AllocaAddress = Builder->CreateGEP(ObjectAddressAndType.first, {
                ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0),
                ConstantInt::get(llvm::Type::getInt32Ty(*Context), 1), // Pointer to Pointer to Data
        });

        auto index = Builder->CreateFPToUI(Index->codegen(), llvm::Type::getInt32Ty(*Context));
        auto Size = Builder->CreateLoad(SizeAddress); // get current size of the list
        auto newSize = Builder->CreateAdd(index, ConstantInt::get(llvm::Type::getInt32Ty(*Context), 1), "new_size");
        auto Condition = Builder->CreateICmpUGT(newSize, Size);
        auto ResizeBlock = BasicBlock::Create(*Context, "resize", Function);
        auto ContinueBlock = BasicBlock::Create(*Context, "continue", Function);
        Builder->CreateCondBr(Condition, ResizeBlock, ContinueBlock);

        Builder->SetInsertPoint(ResizeBlock);
        auto oldAlloca = Builder->CreateLoad(AllocaAddress, "old_alloca");
        auto newAlloca = Builder->CreateAlloca(Object->type->subtype->GetLLVMType(), newSize, "new_alloca"); // create new allocation to expand to size necessary
        auto SizeOfSingleElement = Module->getDataLayout().getTypeAllocSize(Object->type->subtype->GetLLVMType());
        auto SizeInBytes = Builder->CreateMul(Size, ConstantInt::get(llvm::Type::getInt32Ty(*Context), SizeOfSingleElement));
        auto memmoveInstruction = Builder->CreateMemMove(newAlloca, MaybeAlign(), oldAlloca, MaybeAlign(), SizeInBytes);
        Builder->CreateStore(newAlloca, AllocaAddress); // store new address
        Builder->CreateStore(newSize, SizeAddress);   // update size
        Builder->CreateBr(ContinueBlock);

        Builder->SetInsertPoint(ContinueBlock);
        auto Type = Object->type->subtype->GetLLVMType();
        auto Alloca = Builder->CreateLoad(AllocaAddress);
        auto Address = Builder->CreateGEP(Alloca, index);
        return {Address, Type};
    }

    pair<Value *, llvm::Type *> Member::getAddressAndType() {
        auto object = Object->getAddressAndType();
        auto Structure = Symbols.GetStructure(Object->type->type);
        for (int i = 0; i < Structure.members.size(); i++) {
            auto Member = Structure.members[i];
            if (Member.first == Name) {
                auto MemberType = Member.second->GetLLVMType();
                auto MemberPointer = Builder->CreateGEP(object.first,{
                    ConstantInt::get(llvm::Type::getInt32Ty(*Context),0), // 'pierce' through pointer
                    ConstantInt::get(llvm::Type::getInt32Ty(*Context), i)
                });
                return {MemberPointer, MemberType};
            }
        }
        LogError(location, "Member "+Name + " not found in type " + Object->type->type);

    }

    Value *Negative::codegen() {
        auto *Value = expression->codegen();
        if (!Value) {
            return nullptr;
        }
        return Builder->CreateFMul(ConstantFP::get(*Context, APFloat(-1.0)), Value, "neg");
    }

    Value *Number::codegen() {
        return ConstantFP::get(*Context, APFloat(Value));
    }

    Value *Bool::codegen() {
        if (Value)
            return ConstantInt::getTrue(*Context);
        else
            return ConstantInt::getFalse(*Context);
    }

    Value *String::codegen() {
        return Builder->CreateGlobalStringPtr(StringRef(Value));
    }

    Value *Variable::codegen() {
        auto AddressAndType = getAddressAndType();
        return Builder->CreateLoad(AddressAndType.second, AddressAndType.first);
    }

    Value *Indexing::codegen() {
        auto AddressAndType = getAddressAndType();
        return Builder->CreateLoad(AddressAndType.second, AddressAndType.first);
    }

    Value *VariableDefinition::codegen() {
        auto Function = Builder->GetInsertBlock()->getParent();

        auto Alloca = CreateAlloca(Function, type->GetLLVMType(), Name, type->size);
        Symbols.CreateVariable(Name, type, Alloca);
        if (!Value)
            return Builder->CreateStore(Constant::getNullValue(type->GetLLVMType()), Alloca);
        llvm::Value *initialValue;
        initialValue = Value->codegen();
        if (!initialValue)
            return nullptr;
        return Builder->CreateStore(initialValue, Alloca);
    }

    Value *Call::codegen() {
        llvm::Function *function = Module->getFunction(Callee);
        if (!function)
            return LogError(location, "Function not defined!");
        if (function->arg_size() != Arguments.size())
            return LogError(location,
                            "Number of Arguments given does not match the number of arguments of the function.");
        vector<Value *> ArgumentValues = {};
        for (int i = 0; i < Arguments.size(); i++) {
            auto value = Arguments[i]->codegen();
            if (!value)
                return nullptr;
            ArgumentValues.push_back(value);
        }
        return Builder->CreateCall(function, ArgumentValues);
    }

    Value *BinaryExpression::codegen() {
        if (Op == "=") {
            auto AddressAndType = LHS->getAddressAndType();

            auto Value = RHS->codegen();
            if (!Value)
                return nullptr;

            Builder->CreateStore(Value, AddressAndType.first);
            return Value;
        }

        auto L = LHS->codegen();
        auto R = RHS->codegen();

        if (LHS->type->type == "string" && RHS->type->type == "string") {
            if (Op == "=="){
                // FIXME: Make this independent from the string.t file
                if (auto Function = Module->getFunction("isEqual"))
                    return Builder->CreateCall(Function, {L, R});
                else {
                    LogError(location, "Function isEqual not defined! You might have to include the string.t file.");
                    exit(1);
                }
            }
            else{
                LogError(location, "Operator not supported for strings!");
                exit(1);
            }
        } else if (LHS->type->type == "number" && RHS->type->type == "number") {
            if (!L || !R)
                return nullptr;
            if (Op == "+")
                return Builder->CreateFAdd(L, R);
            else if (Op == "-")
                return Builder->CreateFSub(L, R);
            else if (Op == "*")
                return Builder->CreateFMul(L, R);
            else if (Op == "/")
                return Builder->CreateFDiv(L, R);
            else if (Op == "<")
                return Builder->CreateFCmpULT(L, R);
            else if (Op == ">")
                return Builder->CreateFCmpUGT(L, R);
            else if (Op == ">=")
                return Builder->CreateFCmpUGE(L, R);
            else if (Op == "<=")
                return Builder->CreateFCmpULE(L, R);
            else if (Op == "==")
                return Builder->CreateFCmpOEQ(L, R);
            else
                return LogError(location, "Unrecognized Operator.");
        }
        else{
            LogError(location, "Incompatible operator types.");
            exit(1);
        }

    }

    Value *Return::codegen() {
        auto ExpressionValue = Value->codegen();
        if (!ExpressionValue)
            return nullptr;
        return Builder->CreateRet(ExpressionValue);
    }

    Value *IfStatement::codegen() {
        auto ConditionValue = Condition->codegen();
        if (!ConditionValue)
            return nullptr;
        if (ConditionValue->getType() != llvm::Type::getInt1Ty(*Context))
            ConditionValue = Builder->CreateFCmpONE(ConditionValue,
                                                    ConstantFP::get(*Context, APFloat(0.0)));

        auto Function = Builder->GetInsertBlock()->getParent();

        auto ThenBlock = BasicBlock::Create(*Context, "then", Function);
        auto ElseBlock = BasicBlock::Create(*Context, "else");
        auto After = BasicBlock::Create(*Context, "continue");

        BranchInst *conditionInstruction;
        conditionInstruction = Builder->CreateCondBr(ConditionValue, ThenBlock, ElseBlock);

        Builder->SetInsertPoint(ThenBlock);
        Symbols.CreateScope();
        for (auto &Expression: Then) {
            auto ExpressionIR = Expression->codegen();

            if (!ExpressionIR)
                return nullptr;
        }
        if (Builder->GetInsertBlock()->getTerminator() == nullptr)
            Builder->CreateBr(After);
        Symbols.DestroyScope();

        Function->getBasicBlockList().push_back(ElseBlock);
        Builder->SetInsertPoint(ElseBlock);
        Symbols.CreateScope();
        for (auto &Expression: Else) {
            auto ExpressionIR = Expression->codegen();

            if (!ExpressionIR)
                return nullptr;
        }
        if (Builder->GetInsertBlock()->getTerminator() == nullptr)
            Builder->CreateBr(After);
        Symbols.DestroyScope();

        Function->getBasicBlockList().push_back(After);
        Builder->SetInsertPoint(After);
        return conditionInstruction;
    }

    Value *ForLoop::codegen() {
        auto Function = Builder->GetInsertBlock()->getParent();
        auto ForLoopBlock = BasicBlock::Create(*Context, "loop", Function);

        auto StartValue = Start->codegen();
        if (!StartValue)
            return nullptr;

        Symbols.CreateScope();
        auto Alloca = CreateAlloca(Function, llvm::Type::getDoubleTy(*Context), VariableName);
        Symbols.CreateVariable(VariableName, make_shared<Type>("number"), Alloca);
        Builder->CreateStore(StartValue, Alloca);

        Builder->CreateBr(ForLoopBlock);
        Builder->SetInsertPoint(ForLoopBlock);

        for (auto &Expression: Body) {
            auto ExpressionIR = Expression->codegen();
            if (!ExpressionIR)
                return nullptr;
        }

        Value *StepValue;
        if (Step) {
            StepValue = Step->codegen();
            if (!StepValue)
                return nullptr;
        } else {
            StepValue = ConstantFP::get(*Context, APFloat(1.0));
        }
        auto CurrentStep = Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, VariableName);
        auto NextStep = Builder->CreateFAdd(CurrentStep, StepValue, "step");
        Builder->CreateStore(NextStep, Alloca);

        Value *EndCondition = Condition->codegen();
        if (!EndCondition)
            return nullptr;

        if (EndCondition->getType() != llvm::Type::getInt1Ty(*Context))
            EndCondition = Builder->CreateFCmpONE(
                    EndCondition, ConstantFP::get(*Context, APFloat(0.0)), "conditon");

        auto AfterBlock = BasicBlock::Create(*Context, "afterloop", Function);
        Builder->CreateCondBr(EndCondition, ForLoopBlock, AfterBlock);
        Symbols.DestroyScope();
        Builder->SetInsertPoint(AfterBlock);
        return Constant::getNullValue(llvm::Type::getDoubleTy(*Context));
    }

    Value *WhileLoop::codegen() {
        auto Function = Builder->GetInsertBlock()->getParent();
        auto WhileLoopBlock = BasicBlock::Create(*Context, "whileloop", Function);
        auto AfterBlock = BasicBlock::Create(*Context, "afterloop", Function);
        Value *ConditionValue = Condition->codegen();
        if (!ConditionValue)
            return nullptr;

        ConditionValue = Builder->CreateFCmpONE(ConditionValue, ConstantFP::get(*Context, APFloat(0.0)),
                                                "condition");

        Builder->CreateCondBr(ConditionValue, WhileLoopBlock, AfterBlock);
        Builder->SetInsertPoint(WhileLoopBlock);
        Symbols.CreateScope();

        for (auto &Expression: Body) {
            auto ExpressionIR = Expression->codegen();
            if (!ExpressionIR)
                return nullptr;
        }

        ConditionValue = Condition->codegen();
        if (!ConditionValue)
            return nullptr;

        if (ConditionValue->getType() != llvm::Type::getInt1Ty(*Context))
            ConditionValue = Builder->CreateFCmpONE(ConditionValue, ConstantFP::get(*Context, APFloat(0.0)),
                                                    "condition");

        Builder->CreateCondBr(ConditionValue, WhileLoopBlock, AfterBlock);
        Symbols.DestroyScope();

        Builder->SetInsertPoint(AfterBlock);

        return Constant::getNullValue(llvm::Type::getDoubleTy(*Context));
    }

    Value *Function::codegen() {
        llvm::Function *Function = Module->getFunction(Name);
        if (!Function) {
            // Create Vector that specifies the types for the arguments (atm only floating point numbers aka doubles)
            vector<llvm::Type *> ArgumentTypes(Arguments.size());
            for (int i = 0; i < Arguments.size(); i++) {
                ArgumentTypes[i] = Arguments[i].first->GetLLVMType();
            }
            FunctionType *FunctionType = FunctionType::get(type->GetLLVMType(), ArgumentTypes, false);
            Function = llvm::Function::Create(FunctionType, llvm::Function::ExternalLinkage, Name, Module.get());
            int i = 0;
            for (auto &Argument: Function->args()) {
                Argument.setName(Arguments[i].second);
                i += 1;
            }
        }

        if (!Function->empty())
            return LogError(location, "Can't redefine Function");

        //Define BasicBlock to start inserting into for function
        BasicBlock *BasicBlock = BasicBlock::Create(*Context, "entry", Function);
        Builder->SetInsertPoint(BasicBlock);

        Symbols.CreateScope();
        int argument = 0;
        for (auto &Arg: Function->args()) {
            Arg.setName(Arguments[argument].second);
            AllocaInst *Alloca = CreateAlloca(Function, Arguments[argument].first->GetLLVMType(),
                                              Arg.getName().str());
            Symbols.CreateVariable(Arg.getName().str(), Arguments[argument].first, Alloca);
            Builder->CreateStore(&Arg, Alloca);
            argument += 1;
        }
        Symbols.CreateFunction(Name, type, Arguments, Function);
        for (int i = 0; i < Body.size(); i++) {
            auto value = Body[i]->codegen();

            if (!value) {
                Function->eraseFromParent();    // error occurred delete the function
                return Function;
            }
        }
        Symbols.DestroyScope();
        return Function;
    }

    Value *Extern::codegen() {
        llvm::Function *Function = Module->getFunction(Name);
        if (!Function) {
            vector<llvm::Type *> ArgumentTypes(Arguments.size(), llvm::Type::getDoubleTy(*Context));
            for (int i = 0; i < Arguments.size(); i++) {
                ArgumentTypes[i] = Arguments[i].first->GetLLVMType();
            }
            FunctionType *FunctionType = FunctionType::get(type->GetLLVMType(), ArgumentTypes, false);
            Function = llvm::Function::Create(FunctionType, llvm::Function::ExternalLinkage, Name, Module.get());
            int i = 0;
            for (auto &Argument: Function->args()) {
                Argument.setName(Arguments[i].second);
                i += 1;
            }
        }
        Symbols.CreateFunction(Name, type, Arguments, Function);
        return Function;
    }

    Value *Assembly::codegen() {
        assert(false && "Assembly not implemented yet");
        return nullptr;
    }

    llvm::Value *Structure::codegen() {
        vector<llvm::Type *> Types;
        for (auto &Member: Members) {
            auto Type = Member.second->GetLLVMType();
            Types.push_back(Type);
        }
        auto *StructType = llvm::StructType::create(Types, Name);
        Symbols.CreateStructure(Name, Members, StructType);
    }

    Value *Member::codegen() {
        auto AddressAndType = getAddressAndType();
        auto Address = AddressAndType.first;
        auto Type = AddressAndType.second;
        return Builder->CreateLoad(Type, Address);
    }
}