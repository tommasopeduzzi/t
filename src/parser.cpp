//
// Created by tommasopeduzzi on 12/08/2021.
//
#include <memory>
#include <utility>
#include "lexer.h"
#include "parser.h"
#include "error.h"

using namespace std;

namespace t {

    unique_ptr<Node> CheckForNull(unique_ptr<Node> node) {
        if (!node)
            return nullptr;
        return move(node);
    }

    void Parser::ParseFile(string filePath, vector<unique_ptr<Node>> &FunctionDeclarations,
                           vector<unique_ptr<Node>> &TopLevelExpressions, std::set<std::string> &ImportedFiles) {
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
                            CheckForNull(std::move(PrimaryParse()))));
                    break;
            }
        }
    }

    void Parser::HandleImport(std::vector<std::unique_ptr<Node>> &FunctionDeclarations,
                              std::vector<std::unique_ptr<Node>> &TopLevelExpressions,
                              std::set<std::string> &ImportedFiles) {
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

    Token Parser::getNextToken() {
        return CurrentToken = lexer->getToken();
    }

    std::unique_ptr<Type> Parser::ParseType() {
        if (CurrentToken.type != TokenType::TYPE) {
            LogError(lexer->location, "Expected type!");
            exit(1);
        }
        auto TypeString = std::get<std::string>(CurrentToken.value);
        getNextToken();     // eat type
        int size = 1;
        if (CurrentToken == '[') {
            getNextToken();
            // FIXME: handle dynamic array sizes
            if (CurrentToken.type != TokenType::NUMBER) {
                LogError(lexer->location, "Expected int!");
                exit(1);
            }
            size = (int) std::get<double>(CurrentToken.value);
            getNextToken();
            if (CurrentToken != ']') {
                LogError(lexer->location, "Expected ]!");
                exit(1);
            }
            getNextToken();     // eat ']
        }
        if (CurrentToken.type == TokenType::OF_TOKEN) {
            getNextToken(); // eat of

            return std::make_unique<Type>(TypeString, std::move(ParseType()), size);
        }
        return std::make_unique<Type>(TypeString, size);
    }

    std::unique_ptr<Node> Parser::PrimaryParse() {
        switch (CurrentToken.type) {
            case TokenType::RETURN_TOKEN:
                return ParseReturn();
            case TokenType::VAR_TOKEN:
                return ParseVariableDefinition();
            case TokenType::IF_TOKEN:
                return ParseIfStatement();
            case TokenType::FOR_TOKEN:
                return ParseForLoop();
            case TokenType::WHILE_TOKEN:
                return ParseWhileLoop();
            default:
                return ParseBinaryExpression();
        }
    }

    std::unique_ptr<Expression> Parser::ParseExpression() {
        if (CurrentToken == '-')
            return ParseNegative();
        else if (CurrentToken == '(')
            return ParseParentheses();
        switch (CurrentToken.type) {
            case TokenType::IDENTIFIER:
                return ParseIdentifier();
            case TokenType::NUMBER:
                return ParseNumber();
            case TokenType::BOOL:
                return ParseBool();
            case TokenType::STRING:
                return ParseString();
            default:
                LogError(lexer->location, "Unexpected Token");
                getNextToken(); // eat unexpected Token
                return nullptr;
        }
    }

    std::unique_ptr<Expression> Parser::ParseBinaryExpression() {
        auto LHS = ParseExpression();

        if (!LHS) {
            return nullptr;
        }

        return ParseBinaryOperatorRHS(0, std::move(LHS));
    }

    std::unique_ptr<Expression>
    Parser::ParseBinaryOperatorRHS(int expressionPrecedence, std::unique_ptr<Expression> LHS) {
        while (true) {
            if (!std::holds_alternative<std::string>(CurrentToken.value)) {
                return LHS; // it's not a binary operator, because it's value is not string
            }

            if (CurrentToken == '[') {
                // Indexing operation
                getNextToken(); // eat '['
                auto Index = ParseBinaryExpression();
                if (!Index) {
                    return nullptr;
                }
                if (CurrentToken != ']') {
                    LogError(lexer->location, "Expected ']'!");
                    return nullptr;
                }
                getNextToken(); // eat ']'
                auto type = LHS->type;
                LHS = std::make_unique<Indexing>(std::move(LHS), std::move(Index), lexer->location);
                LHS->type = type;   // set type of Indexing Operation to type of  the Object being indexed
            } else {
                std::string Operator = std::get<std::string>(CurrentToken.value);
                int TokenPrecedence = getOperatorPrecedence(Operator);

                if (TokenPrecedence < expressionPrecedence) {
                    return LHS; // It's not a binary expression, just return the left side.
                }

                // Parse right side:
                getNextToken();

                auto RHS = ParseExpression();
                if (!RHS) {
                    return nullptr;
                }
                if (std::holds_alternative<std::string>(CurrentToken.value)) {
                    if (TokenPrecedence < getOperatorPrecedence(std::get<std::string>(CurrentToken.value))) {
                        RHS = ParseBinaryOperatorRHS(TokenPrecedence + 1, std::move(RHS));
                        if (!RHS)
                            return nullptr;
                    }
                }
                // Merge left and right
                LHS = std::make_unique<BinaryExpression>(Operator, std::move(LHS), std::move(RHS), lexer->location);
            }
        }
    }

    std::unique_ptr<Function> Parser::ParseFunction() {
        getNextToken();     // eat 'def'

        if (CurrentToken.type != TokenType::IDENTIFIER) {
            LogError(lexer->location, "Expected Identifier");
            return nullptr;
        }
        std::string Name = std::get<std::string>(CurrentToken.value);

        getNextToken(); // eat Identifier
        if (CurrentToken != '(') {
            LogError(lexer->location, "expected Parentheses");
            return nullptr;
        }
        getNextToken();     // eat '('
        auto Arguments = ParseArgumentDefinition();

        if (CurrentToken.type != TokenType::OPERATOR) {
            LogError(lexer->location, "Expected '->'!");
            return nullptr;
        }
        getNextToken();     // eat '->'
        auto Type = ParseType();

        std::vector<std::unique_ptr<Node>> Body;

        while (CurrentToken.type != TokenType::END_TOKEN) {
            auto Expression = PrimaryParse();

            if (!Expression)     // error parsing expression, return
                return nullptr;

            Body.push_back(std::move(Expression));
        }
        getNextToken();     //eat "end"
        return std::make_unique<Function>(Name, std::move(Type), lexer->location,
                                          std::move(Arguments),
                                          std::move(Body));
    }

    std::unique_ptr<Extern> Parser::ParseExtern() {
        getNextToken(); // eat 'extern'

        if (CurrentToken.type != TokenType::IDENTIFIER) {
            LogError(lexer->location, "Expected Identifier");
            return nullptr;
        }
        std::string Name = std::get<std::string>(CurrentToken.value);
        getNextToken(); // eat Identifier
        if (CurrentToken != '(') {
            LogError(lexer->location, "expected Parentheses");
            return nullptr;
        }
        getNextToken(); // eats '('
        auto Arguments = ParseArgumentDefinition();
        if (CurrentToken.type != TokenType::OPERATOR) {
            LogError(lexer->location, "Expected '->'!");
            return nullptr;
        }
        getNextToken();     // eat '->'

        auto Type = ParseType();
        return std::make_unique<Extern>(Name, std::move(Type), lexer->location, std::move(Arguments));
    }

    std::unique_ptr<IfStatement> Parser::ParseIfStatement() {
        getNextToken();     // eat 'if'

        auto Condition = ParseBinaryExpression();
        if (!Condition)
            return nullptr;

        if (CurrentToken.type != TokenType::DO_TOKEN) {
            LogError(lexer->location, "Expected 'then' after if condition. ");
            return nullptr;
        }
        getNextToken();     // eat 'then'
        std::vector<std::unique_ptr<Node>> Then, Else;
        while (CurrentToken.type != TokenType::END_TOKEN &&
               CurrentToken.type != TokenType::ELSE_TOKEN) {
            auto Expression = PrimaryParse();

            if (!Expression)
                return nullptr;

            Then.push_back(std::move(Expression));
        }
        if (CurrentToken.type == TokenType::ELSE_TOKEN) {
            getNextToken();     //eat 'else'
            while (CurrentToken.type != TokenType::END_TOKEN) {
                auto Expression = PrimaryParse();

                if (!Expression)
                    return nullptr;

                Else.push_back(std::move(Expression));
            }
        }
        getNextToken(); //eat 'end'
        return std::make_unique<IfStatement>(std::move(Condition), std::move(Then), std::move(Else), lexer->location);
    }

    std::unique_ptr<ForLoop> Parser::ParseForLoop() {
        getNextToken(); // eat "for"

        if (CurrentToken.type != TokenType::IDENTIFIER) {
            LogError(lexer->location, "Expected identifier after 'for'!");
            return nullptr;
        }
        std::string VariableName = std::get<std::string>(CurrentToken.value);
        getNextToken(); // eat Identifier
        std::unique_ptr<Expression> StartValue;
        if (CurrentToken == '=') {
            getNextToken();     // eat '='
            StartValue = ParseBinaryExpression();
            if (!StartValue) {
                return nullptr;
            }
        }
        if (CurrentToken != ',') {
            LogError(lexer->location, "Expected ',' after 'for'!");
            return nullptr;
        }
        getNextToken();     // eat ','
        auto Condition = ParseBinaryExpression();
        if (!Condition)
            return nullptr;

        std::unique_ptr<Expression> Step;
        if (CurrentToken == ',') {
            getNextToken();     // eat ','
            Step = ParseBinaryExpression();
            if (!Step)
                return nullptr;
        }
        if (CurrentToken.type != TokenType::DO_TOKEN) {
            LogError(lexer->location, "Expected 'then' after for loop declaration!");
            return nullptr;
        }
        getNextToken();     // eat 'do'
        std::vector<std::unique_ptr<Node>> Body;
        while (CurrentToken.type != TokenType::END_TOKEN) {
            auto Expression = PrimaryParse();

            if (!Expression)
                return nullptr;

            Body.push_back(std::move(Expression));
        }
        getNextToken();     // eat 'end'
        return std::make_unique<ForLoop>(VariableName, std::move(StartValue), std::move(Condition), std::move(Step),
                                         std::move(Body), lexer->location);
    }

    std::unique_ptr<WhileLoop> Parser::ParseWhileLoop() {
        getNextToken();     // eat 'while'

        auto Condition = ParseBinaryExpression();
        if (!Condition)
            return nullptr;

        if (CurrentToken.type != TokenType::DO_TOKEN) {
            LogError(lexer->location, "Expected 'then' after while declaration");
            return nullptr;
        }
        getNextToken();     // eat 'then'
        std::vector<std::unique_ptr<Node>> Body;
        while (CurrentToken.type != TokenType::END_TOKEN) {
            auto Expression = PrimaryParse();
            if (!Expression)
                return nullptr;
            Body.push_back(std::move(Expression));
        }
        getNextToken(); // eat 'end'
        return std::make_unique<WhileLoop>(std::move(Condition), std::move(Body), lexer->location);
    }

    std::unique_ptr<VariableDefinition> Parser::ParseVariableDefinition() {
        getNextToken(); //eat "var"

        auto Type = ParseType();

        if (CurrentToken.type != TokenType::IDENTIFIER) {
            LogError(lexer->location, "Expected identifier after type!");
            return nullptr;
        }

        auto Name = std::get<std::string>(CurrentToken.value);
        getNextToken();     // eat identifier

        if (CurrentToken != '=') {
            return std::make_unique<VariableDefinition>(Name, std::move(Type));
        }

        getNextToken();     // eat '='
        auto Init = ParseBinaryExpression();

        if (!Init)
            return nullptr;     //error already logged

        return std::make_unique<VariableDefinition>(Name, std::move(Init), std::move(Type), lexer->location);
    }

    std::unique_ptr<Negative> Parser::ParseNegative() {
        getNextToken(); //eat '-'
        auto Expression = ParseBinaryExpression();
        if (!Expression)
            return nullptr;
        return std::make_unique<Negative>(std::move(Expression), lexer->location);
    }

    std::unique_ptr<Number> Parser::ParseNumber() {
        auto number = std::make_unique<Number>(std::get<double>(CurrentToken.value), lexer->location);
        getNextToken(); // eat number
        return std::move(number);
    }

    std::unique_ptr<Bool> Parser::ParseBool() {
        auto boolNode = std::make_unique<Bool>(std::get<bool>(CurrentToken.value), lexer->location);
        getNextToken(); // eat bool
        return std::move(boolNode);
    }

    std::unique_ptr<String> Parser::ParseString() {
        auto stringNode = std::make_unique<String>(std::get<std::string>(CurrentToken.value), lexer->location);
        getNextToken(); // eat string
        return std::move(stringNode);
    }

    std::unique_ptr<Expression> Parser::ParseParentheses() {
        getNextToken();
        auto value = ParseBinaryExpression();

        if (!value) {
            return nullptr;
        }

        if (CurrentToken != ')') {
            LogError(lexer->location, "Expected ')' ");
            return nullptr;
        }
        getNextToken();
        return std::move(value);
    }

    std::unique_ptr<Expression> Parser::ParseIdentifier() {
        std::string Name = std::get<std::string>(CurrentToken.value);

        getNextToken(); // eat identifier

        if (CurrentToken != '(')
            // simple variable
            return std::make_unique<Variable>(Name, lexer->location);

        //function call
        getNextToken(); //eat '('
        std::vector<std::unique_ptr<Expression>> Arguments = ParseArguments();

        return std::make_unique<Call>(Name, std::move(Arguments), lexer->location);
    }

    std::unique_ptr<Return> Parser::ParseReturn() {
        getNextToken();     // eat 'return'
        auto Expression = ParseBinaryExpression();
        if (!Expression)
            return nullptr;

        return std::make_unique<Return>(std::move(Expression), lexer->location);
    }

    std::vector<std::unique_ptr<Expression>> Parser::ParseArguments() {
        std::vector<std::unique_ptr<Expression>> Arguments;
        if (CurrentToken == ')') {
            getNextToken();     // eat ')', no arguments
            return Arguments;
        }
        while (CurrentToken != ')') {
            if (auto Argument = ParseBinaryExpression()) {
                Arguments.push_back(std::move(Argument));
            } else {
                LogError(lexer->location, "Invalid Arguments");
                return {};
            }

            if (CurrentToken == ',') {
                getNextToken(); // eat ','
            } else if (CurrentToken == ')') {
                getNextToken(); // eat ')'
                break;          // we are done, end of the arguments
            } else {
                LogError(lexer->location, "Unexpected character");
                return {};
            }
        }
        return Arguments;
    }

    std::vector<std::pair<std::shared_ptr<Type>, std::string>> Parser::ParseArgumentDefinition() {
        std::vector<std::pair<std::shared_ptr<Type>, std::string>> Arguments;
        if (CurrentToken == ')') {
            getNextToken(); // eat ')' in case 0 of arguments.
            return Arguments;
        }
        while (CurrentToken.type == TokenType::TYPE) {
            auto Type = ParseType();
            if (CurrentToken.type != TokenType::IDENTIFIER) {
                LogError(lexer->location, "Expected identifier after type!");
                return {};
            }
            auto Name = std::get<std::string>(CurrentToken.value);
            Arguments.push_back(std::make_pair(std::move(Type), Name));
            getNextToken();     // eat identifier
            if (CurrentToken == ',') {
                getNextToken(); // eat ',' and continue
            } else if (CurrentToken == ')') {
                getNextToken(); // eat ')'
                break;          // we are done, end of the arguments
            } else {
                LogError(lexer->location, "Unexpected Token");
                return {};
            }
        }
        return Arguments;
    }

    int getOperatorPrecedence(std::string Operator) {
        if (OperatorPrecedence[Operator] <= 0)
            return -1;
        return OperatorPrecedence[Operator];
    }
}