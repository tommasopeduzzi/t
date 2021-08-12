//
// Created by tommasopeduzzi on 12/08/2021.
//

#ifndef T_PARSER_H
#define T_PARSER_H

#include "nodes.h"

int CurrentToken;

int getNextToken();
std::unique_ptr<Node> ParseExpression();
std::unique_ptr<Node> ParsePrimaryExpression();
std::unique_ptr<Number> ParseNumber();
std::unique_ptr<Node> ParseParentheses();
std::unique_ptr<Node> ParseIdentifier();
std::vector<std::unique_ptr<Node>> ParseArguments();

#endif //T_PARSER_H
