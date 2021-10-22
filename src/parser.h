//
// Created by tommasopeduzzi on 12/08/2021.
//

#ifndef T_PARSER_H
#define T_PARSER_H

#include <map>
#include "nodes.h"

static std::map<char, int> OperatorPrecedence{
        {'<',1},
        {'>', 1},
        {'+', 2},
        {'-', 2},
        {'/', 3},
        {'*', 3},
};
extern int CurrentToken;
int getNextToken();
std::vector<std::unique_ptr<Node>> ParseProgram();
std::unique_ptr<Node> ParseTopLevelExpression();
std::unique_ptr<Node> ParseExpression();
std::unique_ptr<Node> ParsePrimaryExpression();
std::unique_ptr<Node> ParseBinaryOperatorRHS(int expressionPrecedence, std::unique_ptr<Node> LHS);
std::unique_ptr<Node> ParseFunction();
std::unique_ptr<Node> ParseExtern();
std::unique_ptr<Number> ParseNumber();
std::unique_ptr<Node> ParseParentheses();
std::unique_ptr<Node> ParseIdentifier();
std::vector<std::unique_ptr<Node>> ParseArguments();
std::vector<std::string> ParseArgumentDefinition();
int getOperatorPrecedence();


#endif //T_PARSER_H
