//
// Created by tommasopeduzzi on 12/08/2021.
//
#include <memory>
#include "lexer.h"
#include "parser.h"
#include "nodes.h"
#include "error.h"



int getNextToken(){
    return CurrentToken = getToken();
}

std::unique_ptr<Node> ParseExpression() {
    return nullptr;
}

std::unique_ptr<Node> ParsePrimaryExpression(){
    switch(CurrentToken){
        case identifier:
            return ParseIdentifier();
        case number:
            return ParseNumber();
        case '(':
            return ParseParentheses();
        default:
            LogErrorLineNo("Unexpected Token");
            return nullptr;
    }
}

std::unique_ptr<Number> ParseNumber(){
    auto number = std::make_unique<Number>(NumberValue);
    getNextToken();
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
        LogErrorLineNo("Expected '('");
        return nullptr;
    }
    getNextToken();
    return value;
}

std::unique_ptr<Node> ParseIdentifier(){
    std::string Name = Identifier;

    getNextToken();

    if(CurrentToken != '(')
        // simple variable
        return std::make_unique<Variable>(Name);

    //function call
    getNextToken();
    std::vector<std::unique_ptr<Node>> Arguments = ParseArguments();

    getNextToken();
    return std::make_unique<Call>(Name, std::move(Arguments));
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
        if(getToken() == ',' || CurrentToken == ')')
            getNextToken();
        else{
            LogErrorLineNo("Unexpected character");
            return {};
        }
    }
    return Arguments;
}
