//
// Created by tommasopeduzzi on 11/08/2021.
//

#include <string>

#ifndef T_LEXER_H
#define T_LEXER_H

enum Tokens{
    eof = -1,
    def = -2,
    ext = -3,
    identifier = -4,
    number = -5,
    ret = -6,
    end = -7,
    var = -8,
    if_tok = -9,
    else_tok = -10,
    then_tok = -11,
    for_tok = -12,
    while_tok = -13,
};

int getToken();

static bool isWhiteSpace(char c);

static bool isAlpha(char c);

static bool isAlphaNum(char c);

static bool isDigit(char c);

extern double NumberValue;
extern std::string Identifier;
extern int lineNo;
extern char LastChar;
static std::string digitRegex = "[0-9]";
static std::string alphaNumRegex = "[a-zA-Z0-9]";
static std::string alphaRegex = "[a-zA-Z]";
static std::string whitespaceRegex = "\\s";

#endif //T_LEXER_H
