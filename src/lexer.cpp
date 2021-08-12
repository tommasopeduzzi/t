//
// Created by tommasopeduzzi on 11/08/2021.
//

#include <string>
#include <regex>
#include <iostream>
#include "error.h"
#include "lexer.h"

int getToken(){
    static char LastChar = ' ';

    // Eat up Whitespace
    while(isWhiteSpace(LastChar)){
        LastChar = getchar();
    }

    // Handle Identifiers
    if(isAlpha(LastChar)){
        Identifier += LastChar;
        while(isAlphaNum(LastChar = getchar()))
            Identifier += LastChar;

        if (Identifier == "def")
            return def;
        else if (Identifier == "extern")
            return ext;
        else
            return identifier;
    }

    // Handle Digits
    if(isDigit(LastChar) || LastChar == '.' || LastChar == ','){
        bool decimal;
        std::string NumberString;

        do{
            if(LastChar == '.' || LastChar == '.'){
                if(decimal){
                    LogError(&"Unexpected character: " [ LastChar] );     //TODO: Errors and shit
                    return -1;
                }
                decimal = true;
            }
            NumberString += LastChar;
            LastChar = getchar();
        } while(isDigit(LastChar) || LastChar == '.' || LastChar == ',');

        NumberValue = strtod(NumberString.c_str(), 0);
        return number;
    }

    // Handle comments
    if(LastChar == '#'){
        do{
            LastChar = getchar();
        } while(LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if(LastChar != EOF){
            return getToken();
        }
    }

    // Handle EOF
    if(LastChar == EOF){
        return eof;
    }

    // If something else; returns ascii value
    return int (LastChar);
}

static bool isDigit(char c) {
    return std::regex_match(std::string(1, c), std::regex(digitRegex));
}

static bool isAlphaNum(char c) {
    return std::regex_match(std::string(1, c), std::regex(alphaNumRegex));
}

static bool isAlpha(char c) {
    return std::regex_match(std::string(1, c), std::regex(alphaRegex));
}

static bool isWhiteSpace(char c) {
    return std::regex_match(std::string(1, c), std::regex(whitespaceRegex));
}
