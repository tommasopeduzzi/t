//
// Created by tommasopeduzzi on 11/08/2021.
//

#include <string>
#include <regex>
#include "error.h"
#include "lexer.h"

Lexer::Lexer(std::string filePath){
    this->filePath = filePath;
    file.open(filePath);
    lineNo = 1;
}

char Lexer::getChar(){
    char c = file.get();
    if(file.eof()){
        return EOF;
    }
    if(c == '\n'){
        lineNo++;
    }
    return c;
}

int Lexer::getToken(){
    // Eat up Whitespace
    while(isWhiteSpace(LastChar)){
        LastChar = getChar();
    }

    // Handle Identifiers
    if(isAlpha(LastChar)){
        Identifier = "";
        Identifier += LastChar;
        while(isAlphaNum(LastChar = getChar()))
            Identifier += LastChar;

        if (Identifier == "def")
            return def;
        else if (Identifier == "extern")
            return ext;
        else if (Identifier == "return")
            return ret;
        else if (Identifier == "end")
            return end;
        else if (Identifier == "var")
            return var;
        else if (Identifier == "if")
            return if_tok;
        else if (Identifier == "else")
            return else_tok;
        else if (Identifier == "then")
            return then_tok;
        else if (Identifier == "for")
            return for_tok;
        else if (Identifier == "while")
            return while_tok;
        else if (Identifier == "import")
            return import_tok;
        else if (Types.find(Identifier) != Types.end()){
            Type = Identifier;
            return type;
        }
        else if(Identifier == "true"){
            BoolValue = true;
            return tok_bool;
        }
        else if(Identifier == "false"){
            BoolValue = false;
            return tok_bool;
        }
        else
            return identifier;
    }

    if(LastChar == '"'){
        StringValue = "";
        LastChar = getChar();
        while (LastChar != '"' && LastChar != EOF){
            StringValue += LastChar;
            LastChar = getChar();
        }
        LastChar = getChar();
        return string;
    }

    // Handle comments
    if(LastChar == '#'){
        do{
            LastChar = getChar();
        } while(LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if(LastChar != EOF){
            return getToken();
        }
    }

    // Handle Digits
    if(isDigit(LastChar) || LastChar == '.'){
        bool decimal = false;
        std::string NumberString;

        do{
            if(LastChar == '.'){
                if(decimal){
                    LogError(&"Unexpected character: " [ LastChar] );     //TODO: Errors and shit
                    return -1;
                }
                decimal = true;
            }
            NumberString += LastChar;
            LastChar = getChar();
        } while(isDigit(LastChar) || LastChar == '.');

        NumberValue = strtod(NumberString.c_str(), 0);
        return number;
    }

    // Handle EOF
    if(LastChar == EOF){
        return eof;
    }

    // If something else; returns ascii value
    char returnValue = LastChar;
    LastChar = getChar();
    return int (returnValue);
}

bool Lexer::isDigit(char c) {
    return std::regex_match(std::string(1, c), std::regex(digitRegex));
}

bool Lexer::isAlphaNum(char c) {
    return std::regex_match(std::string(1, c), std::regex(alphaNumRegex));
}

bool Lexer::isAlpha(char c) {
    return std::regex_match(std::string(1, c), std::regex(alphaRegex));
}

bool Lexer::isWhiteSpace(char c) {
    return std::regex_match(std::string(1, c), std::regex(whitespaceRegex));
}
