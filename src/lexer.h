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
    import_tok = -14,
    string = -15,
};



class Lexer {
public:
    double NumberValue;
    std::string StringValue;
    std::string Identifier;
    char LastChar = ' ';
    int lineNo;
    int getToken();
    bool isWhiteSpace(char c);
    bool isAlpha(char c);
    bool isAlphaNum(char c);
    bool isDigit(char c);
};

const std::string digitRegex = "[0-9]";
const std::string alphaNumRegex = "[a-zA-Z0-9]";
const std::string alphaRegex = "[a-zA-Z]";
const std::string whitespaceRegex = "\\s";

#endif //T_LEXER_H
