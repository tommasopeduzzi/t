//
// Created by tommasopeduzzi on 12/08/2021.
//

#ifndef T_PARSER_H
#define T_PARSER_H

#include <map>
#include "nodes.h"
#include "lexer.h"

static std::map<char, int> OperatorPrecedence{
        {'=',1},
        {'<',10},
        {'>', 10},
        {'+', 20},
        {'-', 20},
        {'/', 30},
        {'*', 30},
};

class Parser{
public:
    int CurrentToken;
    int getNextToken();
    std::unique_ptr<Lexer> lexer;
    void ParseFile(std::string filePath, std::vector<std::unique_ptr<Node>> &FunctionDeclarations,
              std::vector<std::unique_ptr<Node>> &TopLevelExpressions, std::set<std::string> &ImportedFiles);
    void HandleImport(std::vector<std::unique_ptr<Node>> &FunctionDeclarations,
                      std::vector<std::unique_ptr<Node>> &TopLevelExpressions, std::set<std::string> &ImportedFiles);
    std::unique_ptr<Node> ParseExpression();
    std::unique_ptr<Node> ParsePrimaryExpression();
    std::unique_ptr<Node> ParseIfStatement();
    std::unique_ptr<Node> ParseForLoop();
    std::unique_ptr<Node> ParseWhileLoop();
    std::unique_ptr<Node> ParseVariableDeclaration();
    std::unique_ptr<Node> ParseBinaryOperatorRHS(int expressionPrecedence, std::unique_ptr<Node> LHS);
    std::unique_ptr<Node> ParseFunction();
    std::unique_ptr<Node> ParseExtern();
    std::unique_ptr<Node> ParseNegative();
    std::unique_ptr<Number> ParseNumber();
    std::unique_ptr<Node> ParseParentheses();
    std::unique_ptr<Node> ParseIdentifier();
    std::unique_ptr<Node> ParseReturnValue();
    std::vector<std::unique_ptr<Node>> ParseArguments();
    std::vector<std::string> ParseArgumentDefinition();
};
int getOperatorPrecedence(char CurrentToken);

#endif //T_PARSER_H
