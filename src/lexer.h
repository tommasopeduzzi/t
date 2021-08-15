//
// Created by tommasopeduzzi on 11/08/2021.
//

#include <string>

#ifndef T_LEXER_H
#define T_LEXER_H

enum Tokens{
    eof = 0,
    def = 1,
    ext = 2,
    identifier = 3,
    number = 4,
    };

int getToken();

static bool isWhiteSpace(char c);

static bool isAlpha(char Char);

static bool isAlphaNum(char Char);

static bool isDigit(char c);

// TODO: Change NumberValue to llvm::Value
extern double NumberValue;
extern std::string Identifier;
extern int lineNo;
static std::string digitRegex = "[0-9]";
static std::string alphaNumRegex = "[a-zA-Z0-9]";
static std::string alphaRegex = "[a-zA-Z]";
static std::string whitespaceRegex = "\\s";

#endif //T_LEXER_H
