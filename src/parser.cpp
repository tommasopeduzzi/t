//
// Created by tommasopeduzzi on 12/08/2021.
//
#include <memory>
#include <utility>
#include "lexer.h"
#include "parser.h"
#include "error.h"

std::unique_ptr<Node> CheckForNull(std::unique_ptr<Node> node){
    if(!node)
        return nullptr;
    return std::move(node);
}

void Parser::ParseFile(std::string filePath, std::vector<std::unique_ptr<Node>> &FunctionDeclarations,
                          std::vector<std::unique_ptr<Node>> &TopLevelExpressions, std::set<std::string> &ImportedFiles){
    lexer = std::make_unique<Lexer>();
    getNextToken();     // get first token
    while (true) {
        switch (CurrentToken) {
            case eof:
                return;
            case def:
                FunctionDeclarations.push_back(std::move(ParseFunction()));
                break;
            case ext:
                FunctionDeclarations.push_back(std::move(
                        CheckForNull(std::move(ParseExtern()))));
                break;
            case import_tok:
                HandleImport(FunctionDeclarations, TopLevelExpressions, ImportedFiles);
                break;
            default:
                TopLevelExpressions.push_back(std::move(
                        CheckForNull(std::move(ParseExpression()))));
                break;
        }
    }
}

void Parser::HandleImport(std::vector<std::unique_ptr<Node>> &FunctionDeclarations,
                          std::vector<std::unique_ptr<Node>> &TopLevelExpressions, std::set<std::string> &ImportedFiles) {
    getNextToken();
    if (CurrentToken != string) {
        std::cerr << "Expected string after import!\n";
        return;
    }
    std::string fileName = lexer->StringValue;
    getNextToken();     // eat string
    auto parser = std::make_unique<Parser>();
    if (ImportedFiles.find(fileName) == ImportedFiles.end()) {
        ImportedFiles.insert(fileName);
        parser->ParseFile(fileName, FunctionDeclarations, TopLevelExpressions, ImportedFiles);
    }
}

int Parser::getNextToken(){
    return CurrentToken = lexer->getToken();
}

std::unique_ptr<Node> Parser::ParseExpression() {
    auto LHS = ParsePrimaryExpression();

    if(!LHS){
        return nullptr;
    }


    return ParseBinaryOperatorRHS(0, std::move(LHS));
}

std::unique_ptr<Node> Parser::ParseBinaryOperatorRHS(int expressionPrecedence, std::unique_ptr<Node> LHS) {
    while(true){
        int TokenPrecedence = getOperatorPrecedence(CurrentToken);
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

        if(TokenPrecedence < getOperatorPrecedence(CurrentToken)){
            RHS = ParseBinaryOperatorRHS(TokenPrecedence + 1, std::move(RHS));
            if(!RHS)
                return nullptr;
        }
        // Merge left and right
        LHS = std::make_unique<BinaryExpression>(Operator, std::move(LHS), std::move(RHS));
    }
}

std::unique_ptr<Node> Parser::ParsePrimaryExpression(){
    switch(this->CurrentToken){
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

std::unique_ptr<Node> Parser::ParseFunction() {
    getNextToken();     // eat 'def'

    if(CurrentToken != identifier){
        LogErrorLineNo("Expected Identifier");
        return nullptr;
    }
    std::string Name = lexer->Identifier;

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

std::unique_ptr<Node> Parser::ParseExtern(){
    getNextToken(); // eat 'extern'

    if(CurrentToken != identifier){
        LogErrorLineNo("Expected Identifier");
        return nullptr;
    }
    std::string Name = lexer->Identifier;
    getNextToken(); // eat Identifier
    if(CurrentToken != '('){
        LogErrorLineNo("expected Parentheses");
        return nullptr;
    }
    getNextToken(); // eats '('
    auto Arguments = ParseArgumentDefinition();
    return std::make_unique<Extern>(Name, Arguments);
}

std::unique_ptr<Node> Parser::ParseIfStatement() {
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

std::unique_ptr<Node> Parser::ParseForLoop(){
    getNextToken(); // eat "for"

    if(CurrentToken != identifier){
        LogErrorLineNo("Expected identifier after 'for'!");
        return nullptr;
    }
    std::string VariableName = lexer->Identifier;
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

std::unique_ptr<Node> Parser::ParseWhileLoop(){
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

std::unique_ptr<Node> Parser::ParseVariableDeclaration() {
    getNextToken(); //eat "var"

    if(CurrentToken != identifier){
        LogErrorLineNo("Expected identifier after 'var'!");
        return nullptr;
    }

    auto Name = lexer->Identifier;
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

std::unique_ptr<Number> Parser::ParseNumber(){
    auto number = std::make_unique<Number>(lexer->NumberValue);
    getNextToken(); // eat number
    return std::move(number);
}

std::unique_ptr<Node> Parser::ParseParentheses(){
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

std::unique_ptr<Node> Parser::ParseIdentifier(){
    std::string Name = lexer->Identifier;

    getNextToken(); // eat identifier

    if(CurrentToken != '(')
        // simple variable
        return std::make_unique<Variable>(Name);

    //function call
    getNextToken(); //eat '('
    std::vector<std::unique_ptr<Node>> Arguments = ParseArguments();

    return std::make_unique<Call>(Name, std::move(Arguments));
}

std::unique_ptr<Node> Parser::ParseReturnValue(){
    getNextToken();     // eat 'return'
    auto Expression = ParseExpression();
    if(!Expression)
        return nullptr;

    return std::make_unique<Return>(std::move(Expression));
}

std::vector<std::unique_ptr<Node>> Parser::ParseArguments() {
    std::vector<std::unique_ptr<Node>> Arguments;
    if(CurrentToken == ')'){
        getNextToken();     // eat ')', no arguments
        return Arguments;
    }
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

std::vector<std::string> Parser::ParseArgumentDefinition() {
    std::vector<std::string> Arguments;
    if (CurrentToken == ')'){
        getNextToken(); // eat ')' in case 0 of arguments.
        return Arguments;
    }
    while(CurrentToken == identifier){
        Arguments.push_back(std::move(lexer->Identifier));
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

int getOperatorPrecedence(char CurrentToken){
    if(!isascii(CurrentToken) || OperatorPrecedence[CurrentToken] <= 0)
        return -1;
    return OperatorPrecedence[CurrentToken];
}