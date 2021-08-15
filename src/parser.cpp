//
// Created by tommasopeduzzi on 12/08/2021.
//
#include <memory>
#include "lexer.h"
#include "parser.h"
#include "error.h"
int CurrentToken;

std::map<char, int> OperatorPrecedence{
    {'<',1},
    {'>', 1},
    {'+', 2},
    {'-', 2},
    {'/', 3},
    {'*', 3},
    };

void Main(){
    getNextToken(); // get the first token
    while(true){
        switch(CurrentToken){
            case eof:
                return;
            case identifier:
                ParseIdentifier();
                break;
            case def:
                ParseFunction();
                break;
            case ext:
                ParseExtern();
                break;
            default:
                ParseTopLevelExpression();
                break;
        }
    }
}

int getNextToken(){
    return CurrentToken = getToken();
}

std::unique_ptr<Node> ParseTopLevelExpression(){
    if(auto Expression = ParseExpression()){
        return std::make_unique<Function>("__anon_expr",
                                          std::vector<std::string>(),
                                          std::move(Expression));
    }
    return nullptr;
}

std::unique_ptr<Node> ParseExpression() {
    auto LHS = ParsePrimaryExpression();

    if(!LHS){
        return nullptr;
    }

    return ParseBinaryOperatorRHS(0, std::move(LHS));
}

std::unique_ptr<Node> ParseBinaryOperatorRHS(int expectedPrecedence, std::unique_ptr<Node> LHS) {
    while(true){
        int TokenPrecedence = getOperatorPrecedence();

        if(TokenPrecedence < expectedPrecedence){
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
        LogErrorLineNo("expected Parenthese");
        return nullptr;
    }
    getNextToken();
    auto Arguments = ParseArgumentDefinition();

    if(auto Body = ParseExpression()){
        return std::make_unique<Function>(Name, std::move(Arguments), std::move(Body));
    }
    return nullptr;
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
        LogErrorLineNo("expected Parenthese");
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
    getNextToken(); //eat '('
    std::vector<std::unique_ptr<Node>> Arguments = ParseArguments();

    return std::make_unique<Call>(Name, std::move(Arguments));
}

std::vector<std::unique_ptr<Node>> ParseArguments() {
    std::vector<std::unique_ptr<Node>> Arguments;
    getNextToken();
    while(CurrentToken != ')'){
        if(auto Argument = ParseExpression()){
            Arguments.push_back(std::move(Argument));
        }
        else{
            LogErrorLineNo("Invalid Arguments");
            return {};
        }
        getNextToken();
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
    if(!isascii(CurrentToken))
        return -1;
    return OperatorPrecedence[CurrentToken] <=0 ? -1 : OperatorPrecedence[CurrentToken];
}