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

std::unique_ptr<Type> Parser::ParseType(){
    if(CurrentToken.type != TokenType::TYPE){
        LogErrorLineNo("Expected type!");
        exit(1);
    }
    auto TypeString = std::get<std::string>(CurrentToken.value);
    getNextToken();     // eat type
    int size = 1;
    if(CurrentToken == '['){
        getNextToken();
        if (CurrentToken.type != TokenType::NUMBER){
            LogErrorLineNo("Expected int!");
            exit(1);
        }
        size = (int)std::get<double>(CurrentToken.value);
        getNextToken();
        if(CurrentToken != ']'){
            LogErrorLineNo("Expected )!");
            exit(1);
        }
    }
    if(CurrentToken.type == TokenType::OF_TOKEN){
        getNextToken(); // eat of

        return std::make_unique<Type>(TypeString, std::move(ParseType()), size);
    }
    return std::make_unique<Type>(TypeString, size);
}

void Parser::ParseFile(std::string filePath, std::vector<std::unique_ptr<Node>> &FunctionDeclarations,
                          std::vector<std::unique_ptr<Node>> &TopLevelExpressions, std::set<std::string> &ImportedFiles){
    lexer = std::make_unique<Lexer>(filePath);
    ImportedFiles.insert(filePath);
    getNextToken();     // get first token
    while (true) {
        switch (CurrentToken.type) {
            case TokenType::EOF_TOKEN:
                return;
            case TokenType::DEF_TOKEN:
                FunctionDeclarations.push_back(std::move(
                        CheckForNull(std::move(ParseFunction()))));
                break;
            case TokenType::EXTERN_TOKEN:
                FunctionDeclarations.push_back(std::move(
                        CheckForNull(std::move(ParseExtern()))));
                break;
            case TokenType::IMPORT_TOKEN:
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
    if (CurrentToken.type != TokenType::STRING) {
        std::cerr << "Expected string after import!\n";
        return;
    }
    std::string fileName = std::get<std::string>(CurrentToken.value);
    getNextToken();     // eat string
    auto parser = std::make_unique<Parser>();
    if (ImportedFiles.find(fileName) == ImportedFiles.end()) {
        parser->ParseFile(fileName, FunctionDeclarations, TopLevelExpressions, ImportedFiles);
    }
}

Token Parser::getNextToken(){
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
        if(!std::holds_alternative<std::string>(CurrentToken.value)){
            return LHS; // it's not a binary operator, because it's value is not string
        }

        if(CurrentToken == '['){
            // Indexing operation
            getNextToken(); // eat '['
            auto Index = ParseExpression();
            if(!Index){
                return nullptr;
            }
            if(CurrentToken != ']'){
                LogErrorLineNo("Expected ']'!");
                return nullptr;
            }
            getNextToken(); // eat ']'
            LHS = std::make_unique<Indexing>(std::move(LHS), std::move(Index));
        }
        else{
            std::string Operator = std::get<std::string>(CurrentToken.value);
            int TokenPrecedence = getOperatorPrecedence(Operator);

            if(TokenPrecedence < expressionPrecedence){
                return LHS; // It's not a binary expression, just return the left side.
            }

            // Parse right side:
            getNextToken();

            auto RHS = ParsePrimaryExpression();
            if(!RHS){
                return nullptr;
            }
            if(std::holds_alternative<std::string>(CurrentToken.value)){
                if(TokenPrecedence < getOperatorPrecedence(std::get<std::string>(CurrentToken.value))){
                    RHS = ParseBinaryOperatorRHS(TokenPrecedence + 1, std::move(RHS));
                    if(!RHS)
                        return nullptr;
                }
            }
            // Merge left and right
            LHS = std::make_unique<BinaryExpression>(Operator, std::move(LHS), std::move(RHS));
        }
    }
}

std::unique_ptr<Node> Parser::ParsePrimaryExpression(){
    if(CurrentToken == '-')
        return ParseNegative();
    else if (CurrentToken == '(')
        return ParseParentheses();
    switch(CurrentToken.type){
        case TokenType::IDENTIFIER:
            return ParseIdentifier();
        case TokenType::NUMBER:
            return ParseNumber();
        case TokenType::RETURN_TOKEN:
            return ParseReturnValue();
        case TokenType::VAR_TOKEN:
            return ParseVariableDeclaration();
        case TokenType::IF_TOKEN:
            return ParseIfStatement();
        case TokenType::FOR_TOKEN:
            return ParseForLoop();
        case TokenType::WHILE_TOKEN:
            return ParseWhileLoop();
        case TokenType::BOOL:
            return ParseBool();
        case TokenType::STRING:
            return ParseString();
        default:
            LogErrorLineNo("Unexpected Token");
            getNextToken();
            return nullptr;
    }
}

std::unique_ptr<Node> Parser::ParseFunction() {
    getNextToken();     // eat 'def'

    if(CurrentToken.type != TokenType::IDENTIFIER){
        LogErrorLineNo("Expected Identifier");
        return nullptr;
    }
    std::string Name = std::get<std::string>(CurrentToken.value);

    getNextToken(); // eat Identifier
    if(CurrentToken != '('){
        LogErrorLineNo("expected Parentheses");
        return nullptr;
    }
    getNextToken();     // eat '('
    auto Arguments = ParseArgumentDefinition();

    if (CurrentToken.type != TokenType::OPERATOR) {
        LogErrorLineNo("Expected '->'!");
        return nullptr;
    }
    getNextToken();     // eat '->'
    auto Type = ParseType();

    std::vector<std::unique_ptr<Node>> Expressions;

    while (CurrentToken.type != TokenType::END_TOKEN){
        auto Expression = ParseExpression();

        if(!Expression)     // error parsing expression, return
            return nullptr;

        Expressions.push_back(std::move(Expression));
    }
    getNextToken();     //eat "end"
    return std::make_unique<Function>(Name, std::move(Type),
                                      std::move(Arguments),
                                      std::move(Expressions));
}

std::unique_ptr<Node> Parser::ParseExtern(){
    getNextToken(); // eat 'extern'

    if(CurrentToken.type != TokenType::IDENTIFIER){
        LogErrorLineNo("Expected Identifier");
        return nullptr;
    }
    std::string Name = std::get<std::string>(CurrentToken.value);
    getNextToken(); // eat Identifier
    if(CurrentToken != '('){
        LogErrorLineNo("expected Parentheses");
        return nullptr;
    }
    getNextToken(); // eats '('
    auto Arguments = ParseArgumentDefinition();
    if (CurrentToken.type != TokenType::OPERATOR) {
        LogErrorLineNo("Expected '->'!");
        return nullptr;
    }
    getNextToken();     // eat '->'

    auto Type = ParseType();
    return std::make_unique<Extern>(Name, std::move(Type), std::move(Arguments));
}

std::unique_ptr<Node> Parser::ParseIfStatement() {
    getNextToken();     // eat 'if'

    auto Condition = ParseExpression();
    if(!Condition)
        return nullptr;

    if(CurrentToken.type != TokenType::DO_TOKEN){
        LogErrorLineNo("Expected 'then' after if condition. ");
        return nullptr;
    }
    getNextToken();     // eat 'then'
    std::vector<std::unique_ptr<Node>> Then, Else;
    while(CurrentToken.type != TokenType::END_TOKEN &&
            CurrentToken.type != TokenType::ELSE_TOKEN){
        auto Expression = ParseExpression();

        if(!Expression)
            return nullptr;

        Then.push_back(std::move(Expression));
    }
    if(CurrentToken.type == TokenType::ELSE_TOKEN){
        getNextToken();     //eat 'else'
        while(CurrentToken.type != TokenType::END_TOKEN){
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

    if(CurrentToken.type != TokenType::IDENTIFIER){
        LogErrorLineNo("Expected identifier after 'for'!");
        return nullptr;
    }
    std::string VariableName = std::get<std::string>(CurrentToken.value);
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
    if(CurrentToken.type != TokenType::DO_TOKEN){
        LogErrorLineNo("Expected 'then' after for loop declaration!");
        return nullptr;
    }
    getNextToken();     // eat 'then'
    std::vector<std::unique_ptr<Node>> Body;
    while (CurrentToken.type != TokenType::END_TOKEN){
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

    if(CurrentToken.type != TokenType::DO_TOKEN){
        LogErrorLineNo("Expected 'then' after while declaration");
        return nullptr;
    }
    getNextToken();     // eat 'then'
    std::vector<std::unique_ptr<Node>> Body;
    while (CurrentToken.type != TokenType::END_TOKEN){
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

    auto Type = ParseType();

    if(CurrentToken.type != TokenType::IDENTIFIER){
        LogErrorLineNo("Expected identifier after type!");
        return nullptr;
    }

    auto Name = std::get<std::string>(CurrentToken.value);
    getNextToken();     // eat identifier

    if(CurrentToken != '='){
        LogErrorLineNo("Expected '=' after identifer. ");
        return nullptr;
    }

    getNextToken();     // eat '='
    auto Init = ParseExpression();

    if(!Init)
        return nullptr;     //error already logged

    return std::make_unique<VariableDefinition>(Name, std::move(Type), std::move(Init));
}

std::unique_ptr<Node> Parser::ParseNegative() {
    getNextToken(); //eat '-'
    auto Expression = ParseExpression();
    if(!Expression)
        return nullptr;
    return std::make_unique<Negative>(std::move(Expression));
}

std::unique_ptr<Number> Parser::ParseNumber(){
    auto number = std::make_unique<Number>(std::get<double>(CurrentToken.value));
    getNextToken(); // eat number
    return std::move(number);
}

std::unique_ptr<Node> Parser::ParseBool(){
    auto boolNode = std::make_unique<Bool>(std::get<bool>(CurrentToken.value));
    getNextToken(); // eat bool
    return std::move(boolNode);
}

std::unique_ptr<Node> Parser::ParseString(){
    auto stringNode = std::make_unique<String>(std::get<std::string>(CurrentToken.value));
    getNextToken(); // eat string
    return std::move(stringNode);
}

std::unique_ptr<Node> Parser::ParseParentheses(){
    getNextToken();
    auto value = ParseExpression();

    if(!value){
        LogErrorLineNo("Error with Parentheses Expression");
        return nullptr;
    }

    if(CurrentToken != ')'){
        LogErrorLineNo("Expected ')' ");
        return nullptr;
    }
    getNextToken();
    return std::move(value);
}

std::unique_ptr<Node> Parser::ParseIdentifier(){
    std::string Name = std::get<std::string>(CurrentToken.value);

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

std::vector<std::pair<std::unique_ptr<Type>,std::string>> Parser::ParseArgumentDefinition() {
    std::vector<std::pair<std::unique_ptr<Type>,std::string>> Arguments;
    if (CurrentToken == ')'){
        getNextToken(); // eat ')' in case 0 of arguments.
        return Arguments;
    }
    while(CurrentToken.type == TokenType::TYPE){
        auto Type = ParseType();
        if(CurrentToken.type != TokenType::IDENTIFIER){
            LogErrorLineNo("Expected identifier after type!");
            return {};
        }
        auto Name = std::get<std::string>(CurrentToken.value);
        Arguments.push_back(std::make_pair(std::move(Type), Name));
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

int getOperatorPrecedence(std::string Operator){
    if(OperatorPrecedence[Operator] <= 0)
        return -1;
    return OperatorPrecedence[Operator];
}