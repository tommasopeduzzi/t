//
// Created by tommasopeduzzi on 12/08/2021.
//

#pragma once

#include <map>
#include "nodes.h"
#include "lexer.h"

using namespace std;

namespace t {

    static map<string, int> OperatorPrecedence{
            {"=",  1},
            {"<",  10},
            {"<=", 10},
            {">=", 10},
            {"==", 10},
            {"<",  10},
            {">",  10},
            {"+",  20},
            {"-",  20},
            {"/",  30},
            {"*",  30},
    };

    class Parser {
    public:
        Token CurrentToken;

        Token getNextToken();

        unique_ptr<Lexer> lexer;

        void ParseFile(string filePath, vector<unique_ptr<Node>> &FunctionDeclarations,
                       vector<unique_ptr<Node>> &TopLevelExpressions, set<string> &ImportedFiles);

        void HandleImport(vector<unique_ptr<Node>> &FunctionDeclarations,
                          vector<unique_ptr<Node>> &TopLevelExpressions,
                          set<string> &ImportedFiles);

        unique_ptr<Node> PrimaryParse();

        unique_ptr<IfStatement> ParseIfStatement();

        unique_ptr<ForLoop> ParseForLoop();

        unique_ptr<WhileLoop> ParseWhileLoop();

        unique_ptr<VariableDefinition> ParseVariableDefinition();

        unique_ptr<Expression> ParseExpression();

        unique_ptr<Expression> ParseBinaryExpression();

        unique_ptr<Expression> ParseBinaryOperatorRHS(int expressionPrecedence, unique_ptr<Expression> LHS);

        unique_ptr<Function> ParseFunction();

        unique_ptr<Extern> ParseExtern();

        unique_ptr<Negative> ParseNegative();

        unique_ptr<Number> ParseNumber();

        unique_ptr<Bool> ParseBool();

        unique_ptr<String> ParseString();

        unique_ptr<Expression> ParseParentheses();

        unique_ptr<Expression> ParseIdentifier();

        unique_ptr<Return> ParseReturn();

        vector<unique_ptr<Expression>> ParseArguments();

        vector<pair<shared_ptr<Type>, string>> ParseArgumentDefinition();

        unique_ptr<Type> ParseType();

        unique_ptr<Assembly> ParseAssembly();

    };

    int getOperatorPrecedence(string Operator);

}