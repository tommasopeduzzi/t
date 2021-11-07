//
// Created by tommasopeduzzi on 12/08/2021.
//
#include <memory>
#include "lexer.h"
#include "parser.h"
#include "error.h"
#include "codegen.h"

int CurrentToken;

int getNextToken(){
    return CurrentToken = getToken();
}

std::unique_ptr<Node> ParseExpression() {
    auto LHS = ParsePrimaryExpression();

    if(!LHS){
        return nullptr;
    }


    return ParseBinaryOperatorRHS(0, std::move(LHS));
}

std::unique_ptr<Node> ParseBinaryOperatorRHS(int expressionPrecedence, std::unique_ptr<Node> LHS) {
    while(true){
        int TokenPrecedence = getOperatorPrecedence();
        if(TokenPrecedence < expressionPrecedence){
            return LHS; // It's not a binary expression, just return the left side.
        }

        int Operator = CurrentToken;

        // Parse right side:
        getNextToken();

        auto RHS = ParsePrimaryExpression();
        if(!RHS){
            return nullptr;
        }

        if(TokenPrecedence < getOperatorPrecedence()){
            RHS = ParseBinaryOperatorRHS(TokenPrecedence + 1, std::move(RHS));
            if(!RHS)
                return nullptr;
        }
        // Merge left and right
        LHS = std::make_unique<BinaryExpression>(Operator, std::move(LHS), std::move(RHS));
    }
}

std::unique_ptr<Node> ParsePrimaryExpression(){
    switch(CurrentToken){
        case identifier:
            return ParseIdentifier();
        case number:
            return ParseNumber();
        case '(':
            return ParseParentheses();
        case ret:
            return ParseReturnValue();
        case var:
            return ParseVariableDeclaration();
        case if_tok:
            return ParseIfStatement();
        case for_tok:
            return ParseForLoop();
        case while_tok:
            return ParseWhileLoop();
        default:
            LogErrorLineNo("Unexpected Token");
            return nullptr;
    }
}

std::unique_ptr<Node> ParseFunction() {
    getNextToken();     // eat 'def'

    if(CurrentToken != identifier){
        LogErrorLineNo("Expected Identifier");
        return nullptr;
    }
    std::string Name = Identifier;

    getNextToken(); // eat Identifier
    if(CurrentToken != '('){
        LogErrorLineNo("expected Parentheses");
        return nullptr;
    }
    getNextToken();     // eat '('
    auto Arguments = ParseArgumentDefinition();

    std::vector<std::unique_ptr<Node>> Expressions;

    while (CurrentToken != end){
        auto Expression = ParseExpression();

        if(!Expression)     // error parsing expression, return
            return nullptr;

        Expressions.push_back(std::move(Expression));
    }
    getNextToken();     //eat "end"
    return std::make_unique<Function>(Name,
                                      Arguments,
                                      std::move(Expressions));
}

std::unique_ptr<Node> ParseExtern(){
    getNextToken(); // eat 'extern'

    if(CurrentToken != identifier){
        LogErrorLineNo("Expected Identifier");
        return nullptr;
    }
    std::string Name = Identifier;
    getNextToken(); // eat Identifier
    if(CurrentToken != '('){
        LogErrorLineNo("expected Parentheses");
        return nullptr;
    }
    getNextToken(); // eats '('
    auto Arguments = ParseArgumentDefinition();
    return std::make_unique<Extern>(Name, Arguments);
}

std::unique_ptr<Node> ParseIfStatement() {
    getNextToken();     // eat 'if'

    auto Condition = ParseExpression();
    if(!Condition)
        return nullptr;

    if(CurrentToken != then_tok){
        LogErrorLineNo("Expected 'then' after if condition. ");
        return nullptr;
    }
    getNextToken();     // eat 'then'
    std::vector<std::unique_ptr<Node>> Then, Else;
    while(CurrentToken != end && CurrentToken != else_tok){
        auto Expression = ParseExpression();

        if(!Expression)
            return nullptr;

        Then.push_back(std::move(Expression));
    }
    if(CurrentToken == else_tok){
        getNextToken();     //eat 'else'
        while(CurrentToken != end){
            auto Expression = ParseExpression();

            if(!Expression)
                return nullptr;

            Else.push_back(std::move(Expression));
        }
    }
    getNextToken(); //eat 'end'
    return std::make_unique<IfExpression>(std::move(Condition), std::move(Then), std::move(Else));
}

std::unique_ptr<Node> ParseForLoop(){
    getNextToken(); // eat "for"

    if(CurrentToken != identifier){
        LogErrorLineNo("Expected identifier after 'for'!");
        return nullptr;
    }
    std::string VariableName = Identifier;
    getNextToken(); // eat Identifier
    std::unique_ptr<Node> StartValue;
    if(CurrentToken == '='){
        getNextToken();     // eat '='
        StartValue = ParseExpression();
        if(!StartValue){
            return nullptr;
        }
    }
    if(CurrentToken != ','){
        LogErrorLineNo("Expected ',' after 'for'!");
        return nullptr;
    }
    getNextToken();     // eat ','
    auto Condition = ParseExpression();
    if(!Condition)
        return nullptr;

    std::unique_ptr<Node> Step;
    if(CurrentToken == ','){
        getNextToken();     // eat ','
        Step = ParseExpression();
        if(!Step)
            return nullptr;
    }
    if(CurrentToken != then_tok){
        LogErrorLineNo("Expected 'then' after for loop declaration!");
        return nullptr;
    }
    getNextToken();     // eat 'then'
    std::vector<std::unique_ptr<Node>> Body;
    while (CurrentToken != end){
        auto Expression = ParseExpression();

        if(!Expression)
            return nullptr;

        Body.push_back(std::move(Expression));
    }
    getNextToken();     // eat 'end'
    return std::make_unique<ForLoop>(VariableName, std::move(StartValue), std::move(Condition), std::move(Step), std::move(Body));
}

std::unique_ptr<Node> ParseWhileLoop(){
    getNextToken();     // eat 'while'

    auto Condition = ParseExpression();
    if(!Condition)
        return nullptr;

    if(CurrentToken != then_tok){
        LogErrorLineNo("Expected 'then' after while declaration");
        return nullptr;
    }
    getNextToken();     // eat 'then'
    std::vector<std::unique_ptr<Node>> Body;
    while (CurrentToken != end){
        auto Expression = ParseExpression();
        if(!Expression)
            return nullptr;
        Body.push_back(std::move(Expression));
    }
    getNextToken(); // eat 'end'
    return std::make_unique<WhileLoop>(std::move(Condition), std::move(Body));
}

std::unique_ptr<Node> ParseVariableDeclaration() {
    getNextToken(); //eat "var"

    if(CurrentToken != identifier){
        LogErrorLineNo("Expected identifier after 'var'!");
        return nullptr;
    }

    auto Name = Identifier;
    getNextToken();     // eat identifier

    if(CurrentToken != '='){
        LogErrorLineNo("Expected '=' after identifer. ");
        return nullptr;
    }

    getNextToken();     // eat '='
    auto Init = ParseExpression();

    if(!Init)
        return nullptr;     //error already logged

    return std::make_unique<VariableDefinition>(Name, std::move(Init));
}

std::unique_ptr<Number> ParseNumber(){
    auto number = std::make_unique<Number>(NumberValue);
    getNextToken(); // eat number
    return std::move(number);
}

std::unique_ptr<Node> ParseParentheses(){
    getNextToken();
    auto value = ParseExpression();

    if(!value){
        LogErrorLineNo("Error with Parentheses Expression");
        return nullptr;
    }

    if(CurrentToken != '('){
        LogErrorLineNo("Expected '(' ");
        return nullptr;
    }
    getNextToken();
    return value;
}

std::unique_ptr<Node> ParseIdentifier(){
    std::string Name = Identifier;

    getNextToken(); // eat identifier

    if(CurrentToken != '(')
        // simple variable
        return std::make_unique<Variable>(Name);

    //function call
    getNextToken(); //eat '('
    std::vector<std::unique_ptr<Node>> Arguments = ParseArguments();

    return std::make_unique<Call>(Name, std::move(Arguments));
}

std::unique_ptr<Node> ParseReturnValue(){
    getNextToken();     // eat 'return'
    auto Expression = ParseExpression();
    if(!Expression)
        return nullptr;

    return std::make_unique<Return>(std::move(Expression));
}

std::vector<std::unique_ptr<Node>> ParseArguments() {
    std::vector<std::unique_ptr<Node>> Arguments;
    while(CurrentToken != ')'){
        if(auto Argument = ParseExpression()){
            Arguments.push_back(std::move(Argument));
        }
        else{
            LogErrorLineNo("Invalid Arguments");
            return {};
        }

        if(CurrentToken == ',' ){
            getNextToken(); // eat ','
        }
        else if(CurrentToken == ')')
        {
            getNextToken(); // eat ')'
            break;          // we are done, end of the arguments
        }
        else{
            LogErrorLineNo("Unexpected character");
            return {};
        }
    }
    return Arguments;
}

std::vector<std::string> ParseArgumentDefinition() {
    std::vector<std::string> Arguments;
    while(CurrentToken == identifier){
        Arguments.push_back(std::move(Identifier));
        getNextToken();     // eat identifier
        if(CurrentToken == ',' ){
            getNextToken(); // eat ',' and continue
        }
        else if(CurrentToken == ')')
        {
            getNextToken(); // eat ')'
            break;          // we are done, end of the arguments
        }
        else{
            LogErrorLineNo("Unexpected character");
            return {};
        }
    }
    return Arguments;
}

int getOperatorPrecedence(){
    if(!isascii(CurrentToken) || OperatorPrecedence[CurrentToken] <= 0)
        return -1;
    return OperatorPrecedence[CurrentToken];
}