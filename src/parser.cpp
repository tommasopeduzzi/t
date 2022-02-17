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
                           vector<unique_ptr<Node>> &TopLevelExpressions,
                           vector<unique_ptr<Structure>> &Structures,
                           set<string> &ImportedFiles) {
        lexer = make_unique<Lexer>(filePath);
        ImportedFiles.insert(filePath);
        getNextToken();     // get first token
        while (true) {
            switch (CurrentToken.type) {
                case TokenType::EOF_TOKEN:
                    return;
                case TokenType::DEF_TOKEN:
                    FunctionDeclarations.push_back(move(ParseFunction()));
                    break;
                case TokenType::EXTERN_TOKEN:
                    FunctionDeclarations.push_back(move(ParseExtern()));
                    break;
                case TokenType::IMPORT_TOKEN:
                    HandleImport(FunctionDeclarations, TopLevelExpressions, Structures, ImportedFiles);
                    break;
                case TokenType::STRUCT_TOKEN:
                    Structures.push_back(move(ParseStructure()));
                    break;
                default:
                    TopLevelExpressions.push_back(move(PrimaryParse()));
                    break;
            }
        }
    }

    void Parser::HandleImport(vector<unique_ptr<Node>> &FunctionDeclarations,
                              vector<unique_ptr<Node>> &TopLevelExpressions,
                              vector<unique_ptr<Structure>> &Structures,
                              set<string> &ImportedFiles) {
        getNextToken();
        if (CurrentToken.type != TokenType::STRING) {
            cerr << "Expected string after import!\n";
            return;
        }
        string filePath = get<string>(CurrentToken.value);
        if(!filesystem::path(filePath).is_absolute())
            filePath = filesystem::canonical(lexer->location.file + "/" + filePath).string();
        getNextToken();     // eat string
        auto parser = make_unique<Parser>();
        if (ImportedFiles.find(filePath) == ImportedFiles.end()) {
            parser->ParseFile(filePath, FunctionDeclarations, TopLevelExpressions, Structures, ImportedFiles);
        }
    }

    Token Parser::getNextToken() {
        return CurrentToken = lexer->getToken();
    }

    unique_ptr<Type> Parser::ParseType() {
        if (CurrentToken.type != TokenType::TYPE) {
            LogError(lexer->location, "Expected type!");
            exit(1);
        }
        auto TypeString = get<string>(CurrentToken.value);
        getNextToken();     // eat type
        int size = 1;
        if (CurrentToken == '[') {
            getNextToken();
            // FIXME: handle dynamic array sizes
            if (CurrentToken.type != TokenType::NUMBER) {
                LogError(lexer->location, "Expected int!");
                exit(1);
            }
            size = (int) get<double>(CurrentToken.value);
            getNextToken();
            if (CurrentToken != ']') {
                LogError(lexer->location, "Expected ]!");
                exit(1);
            }
            getNextToken();     // eat ']
        }
        if (CurrentToken.type == TokenType::OF_TOKEN) {
            getNextToken(); // eat of

            return make_unique<Type>(TypeString, move(ParseType()), size);
        }
        return make_unique<Type>(TypeString, size);
    }

    unique_ptr<Node> Parser::PrimaryParse() {
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
            case TokenType::ASM_TOKEN:
                return ParseAssembly();
            default:
                return ParseBinaryExpression();
        }
    }

    unique_ptr<Expression> Parser::ParseExpression() {
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

    unique_ptr<Expression> Parser::ParseBinaryExpression() {
        auto LHS = ParseExpression();

        if (!LHS) {
            return nullptr;
        }

        return ParseBinaryOperatorRHS(0, move(LHS));
    }

    unique_ptr<Expression>
    Parser::ParseBinaryOperatorRHS(int expressionPrecedence, unique_ptr<Expression> LHS) {
        while (true) {
            if (!holds_alternative<string>(CurrentToken.value)) {
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
                LHS = make_unique<Indexing>(move(LHS), move(Index), lexer->location);
                LHS->type = type;   // set type of Indexing Operation to type of  the Object being indexed
            } else {
                string Operator = get<string>(CurrentToken.value);
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
                if (holds_alternative<string>(CurrentToken.value)) {
                    if (TokenPrecedence < getOperatorPrecedence(get<string>(CurrentToken.value))) {
                        RHS = ParseBinaryOperatorRHS(TokenPrecedence + 1, move(RHS));
                        if (!RHS)
                            return nullptr;
                    }
                }
                // Merge left and right
                LHS = make_unique<BinaryExpression>(Operator, move(LHS), move(RHS), lexer->location);
            }
        }
    }

    unique_ptr<Function> Parser::ParseFunction() {
        getNextToken();     // eat 'def'

        if (CurrentToken.type != TokenType::IDENTIFIER) {
            LogError(lexer->location, "Expected Identifier");
            return nullptr;
        }
        string Name = get<string>(CurrentToken.value);

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

        vector<unique_ptr<Node>> Body;

        while (CurrentToken.type != TokenType::END_TOKEN) {
            auto Expression = PrimaryParse();

            if (!Expression)     // error parsing expression, return
                return nullptr;

            Body.push_back(move(Expression));
        }
        getNextToken();     //eat "end"
        return make_unique<Function>(Name, move(Type), lexer->location,
                                          move(Arguments),
                                          move(Body));
    }

    unique_ptr<Extern> Parser::ParseExtern() {
        getNextToken(); // eat 'extern'

        if (CurrentToken.type != TokenType::IDENTIFIER) {
            LogError(lexer->location, "Expected Identifier");
            return nullptr;
        }
        string Name = get<string>(CurrentToken.value);
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
        return make_unique<Extern>(Name, move(Type), lexer->location, move(Arguments));
    }

    unique_ptr<IfStatement> Parser::ParseIfStatement() {
        getNextToken();     // eat 'if'

        auto Condition = ParseBinaryExpression();
        if (!Condition)
            return nullptr;

        if (CurrentToken.type != TokenType::DO_TOKEN) {
            LogError(lexer->location, "Expected 'then' after if condition. ");
            return nullptr;
        }
        getNextToken();     // eat 'then'
        vector<unique_ptr<Node>> Then, Else;
        while (CurrentToken.type != TokenType::END_TOKEN &&
               CurrentToken.type != TokenType::ELSE_TOKEN) {
            auto Expression = PrimaryParse();

            if (!Expression)
                return nullptr;

            Then.push_back(move(Expression));
        }
        if (CurrentToken.type == TokenType::ELSE_TOKEN) {
            getNextToken();     //eat 'else'
            while (CurrentToken.type != TokenType::END_TOKEN) {
                auto Expression = PrimaryParse();

                if (!Expression)
                    return nullptr;

                Else.push_back(move(Expression));
            }
        }
        getNextToken(); //eat 'end'
        return make_unique<IfStatement>(move(Condition), move(Then), move(Else), lexer->location);
    }

    unique_ptr<ForLoop> Parser::ParseForLoop() {
        getNextToken(); // eat "for"

        if (CurrentToken.type != TokenType::IDENTIFIER) {
            LogError(lexer->location, "Expected identifier after 'for'!");
            return nullptr;
        }
        string VariableName = get<string>(CurrentToken.value);
        getNextToken(); // eat Identifier
        unique_ptr<Expression> StartValue;
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

        unique_ptr<Expression> Step;
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
        vector<unique_ptr<Node>> Body;
        while (CurrentToken.type != TokenType::END_TOKEN) {
            auto Expression = PrimaryParse();

            if (!Expression)
                return nullptr;

            Body.push_back(move(Expression));
        }
        getNextToken();     // eat 'end'
        return make_unique<ForLoop>(VariableName, move(StartValue), move(Condition), move(Step),
                                         move(Body), lexer->location);
    }

    unique_ptr<WhileLoop> Parser::ParseWhileLoop() {
        getNextToken();     // eat 'while'

        auto Condition = ParseBinaryExpression();
        if (!Condition)
            return nullptr;

        if (CurrentToken.type != TokenType::DO_TOKEN) {
            LogError(lexer->location, "Expected 'then' after while declaration");
            return nullptr;
        }
        getNextToken();     // eat 'then'
        vector<unique_ptr<Node>> Body;
        while (CurrentToken.type != TokenType::END_TOKEN) {
            auto Expression = PrimaryParse();
            if (!Expression)
                return nullptr;
            Body.push_back(move(Expression));
        }
        getNextToken(); // eat 'end'
        return make_unique<WhileLoop>(move(Condition), move(Body), lexer->location);
    }

    unique_ptr<VariableDefinition> Parser::ParseVariableDefinition() {
        getNextToken(); //eat "var"

        auto Type = ParseType();

        if (CurrentToken.type != TokenType::IDENTIFIER) {
            LogError(lexer->location, "Expected identifier after type!");
            return nullptr;
        }

        auto Name = get<string>(CurrentToken.value);
        getNextToken();     // eat identifier

        if (CurrentToken != '=') {
            return make_unique<VariableDefinition>(Name, move(Type));
        }

        getNextToken();     // eat '='
        auto Init = ParseBinaryExpression();

        if (!Init)
            return nullptr;     //error already logged

        return make_unique<VariableDefinition>(Name, move(Init), move(Type), lexer->location);
    }

    unique_ptr<Negative> Parser::ParseNegative() {
        getNextToken(); //eat '-'
        auto Expression = ParseBinaryExpression();
        if (!Expression)
            return nullptr;
        return make_unique<Negative>(move(Expression), lexer->location);
    }

    unique_ptr<Number> Parser::ParseNumber() {
        auto number = make_unique<Number>(get<double>(CurrentToken.value), lexer->location);
        getNextToken(); // eat number
        return move(number);
    }

    unique_ptr<Bool> Parser::ParseBool() {
        auto boolNode = make_unique<Bool>(get<bool>(CurrentToken.value), lexer->location);
        getNextToken(); // eat bool
        return move(boolNode);
    }

    unique_ptr<String> Parser::ParseString() {
        auto stringNode = make_unique<String>(get<string>(CurrentToken.value), lexer->location);
        getNextToken(); // eat string
        return move(stringNode);
    }

    unique_ptr<Structure> Parser::ParseStructure() {
        map<string, shared_ptr<Type>> Members = {};
        getNextToken(); // eat 'struct'
        if (CurrentToken.type != TokenType::IDENTIFIER) {
            LogError(lexer->location, "Expected identifier after 'struct'!");
            return nullptr;
        }
        string Name = get<string>(CurrentToken.value);
        getNextToken();
        while (CurrentToken.type != TokenType::END_TOKEN) {
            if (CurrentToken.type != TokenType::TYPE){
                LogError(lexer->location, "Expected type declaration!");
                return nullptr;
            }
            auto Type = ParseType();
            if (CurrentToken.type != TokenType::IDENTIFIER) {
                LogError(lexer->location, "Expected identifier after type!");
                return nullptr;
            }
            auto MemberName = get<string>(CurrentToken.value);
            Members[MemberName] = move(Type);
            getNextToken();
        }
        getNextToken(); // eat 'end'
        lexer->Types.insert(Name);
        return make_unique<Structure>(Name, move(Members), lexer->location);
    }

    unique_ptr<Expression> Parser::ParseParentheses() {
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
        return move(value);
    }

    unique_ptr<Assembly> Parser::ParseAssembly(){
        getNextToken(); // eat 'asm'
        if (CurrentToken.type != TokenType::STRING){
            LogError(lexer->location, "Expected string after 'asm'");
            exit(1);
        }
        auto assembly = make_unique<Assembly>(get<string>(CurrentToken.value), lexer->location);
        getNextToken(); // eat string
        return move(assembly);
    }

    unique_ptr<Expression> Parser::ParseIdentifier() {
        string Name = get<string>(CurrentToken.value);

        getNextToken(); // eat identifier

        if (CurrentToken != '(')
            // simple variable
            return make_unique<Variable>(Name, lexer->location);

        //function call
        getNextToken(); //eat '('
        vector<unique_ptr<Expression>> Arguments = ParseArguments();

        return make_unique<Call>(Name, move(Arguments), lexer->location);
    }

    unique_ptr<Return> Parser::ParseReturn() {
        getNextToken();     // eat 'return'
        auto Expression = ParseBinaryExpression();
        if (!Expression)
            return nullptr;

        return make_unique<Return>(move(Expression), lexer->location);
    }

    vector<unique_ptr<Expression>> Parser::ParseArguments() {
        vector<unique_ptr<Expression>> Arguments;
        if (CurrentToken == ')') {
            getNextToken();     // eat ')', no arguments
            return Arguments;
        }
        while (CurrentToken != ')') {
            if (auto Argument = ParseBinaryExpression()) {
                Arguments.push_back(move(Argument));
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

    vector<pair<shared_ptr<Type>, string>> Parser::ParseArgumentDefinition() {
        vector<pair<shared_ptr<Type>, string>> Arguments;
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
            auto Name = get<string>(CurrentToken.value);
            Arguments.push_back(make_pair(move(Type), Name));
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

    int getOperatorPrecedence(string Operator) {
        if (OperatorPrecedence[Operator] <= 0)
            return -1;
        return OperatorPrecedence[Operator];
    }
}