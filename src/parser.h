//
// Created by tommasopeduzzi on 12/08/2021.
//

#ifndef T_PARSER_H
#define T_PARSER_H

#include <map>
#include "nodes.h"
#include "lexer.h"

static std::map<std::string, int> OperatorPrecedence{
        {"=",1},
        {"<",10},
        {"<=",10},
        {">=",10},
        {"==",10},
        {"<",10},
        {">", 10},
        {"+", 20},
        {"-", 20},
        {"/", 30},
        {"*", 30},
};

class Parser{
public:
    Token CurrentToken;
    Token getNextToken();
    std::unique_ptr<Lexer> lexer;
    void ParseFile(std::string filePath, std::vector<std::unique_ptr<Node>> &FunctionDeclarations,
              std::vector<std::unique_ptr<Node>> &TopLevelExpressions, std::set<std::string> &ImportedFiles);
    void HandleImport(std::vector<std::unique_ptr<Node>> &FunctionDeclarations,
                      std::vector<std::unique_ptr<Node>> &TopLevelExpressions, std::set<std::string> &ImportedFiles);
    std::unique_ptr<Node> PrimaryParse();
    std::unique_ptr<IfStatement> ParseIfStatement();
    std::unique_ptr<ForLoop> ParseForLoop();
    std::unique_ptr<WhileLoop> ParseWhileLoop();
    std::unique_ptr<VariableDefinition> ParseVariableDefinition();
    std::unique_ptr<Expression> ParseExpression();
    std::unique_ptr<Expression> ParseBinaryExpression();
    std::unique_ptr<Expression> ParseBinaryOperatorRHS(int expressionPrecedence, std::unique_ptr<Expression> LHS);
    std::unique_ptr<Function> ParseFunction();
    std::unique_ptr<Extern> ParseExtern();
    std::unique_ptr<Negative> ParseNegative();
    std::unique_ptr<Number> ParseNumber();
    std::unique_ptr<Bool> ParseBool();
    std::unique_ptr<String> ParseString();
    std::unique_ptr<Expression> ParseParentheses();
    std::unique_ptr<Expression> ParseIdentifier();
    std::unique_ptr<Return> ParseReturn();
    std::vector<std::unique_ptr<Expression>> ParseArguments();
    std::vector<std::pair<std::unique_ptr<Type>,std::string>> ParseArgumentDefinition();
    std::unique_ptr<Type> ParseType();

};
int getOperatorPrecedence(std::string Operator);

#endif //T_PARSER_H
