//
// Created by tommasopeduzzi on 12/08/2021.
//
#include <memory>
#include "lexer.h"
#include "parser.h"
#include "error.h"
int CurrentToken;

int getNextToken(){
    return CurrentToken = getToken();
}

std::unique_ptr<Node> ParseExpression() {
    auto LHS = ParsePrimaryExpression();

    if(!LHS){
        return nullptr;
    }

    // I have to save if it's a return statement, because if not it gets lost...
    // There has to be a better way to do this
    bool returnValue = LHS->returnValue;
    auto BinaryExpression =  ParseBinaryOperatorRHS(0, std::move(LHS));
    BinaryExpression->returnValue = returnValue;
    return BinaryExpression;
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
        default:
            LogErrorLineNo("Unexpected Token");
            return nullptr;
    }
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
    getNextToken();     // eat "return"
    auto Node = ParsePrimaryExpression();
    Node->returnValue = true;
    return Node;
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