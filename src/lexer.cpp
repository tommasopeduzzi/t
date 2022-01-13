//
// Created by tommasopeduzzi on 11/08/2021.
//

#include <string>
#include <regex>
#include "error.h"
#include "lexer.h"

namespace t {

    bool operator==(const Token &lhs, const char c) {
        return std::holds_alternative<std::string>(lhs.value) && std::get<std::string>(lhs.value) == std::string(1, c);
    }

    bool operator!=(const Token &lhs, const char c) {
        return !(lhs == c);
    }

    Lexer::Lexer(std::string filePath) {
        file.open(filePath);
        lineNo = 1;
    }

    char Lexer::getChar() {
        char c = file.get();
        if (file.eof()) {
            return EOF;
        }
        if (c == '\n') {
            lineNo++;
        }
        return c;
    }

    Token Lexer::getToken() {
        // Eat up Whitespace
        while (isWhiteSpace(LastChar)) {
            LastChar = getChar();
        }

        // Handle Identifiers
        if (isAlpha(LastChar)) {
            std::string Token = "";
            Token += LastChar;
            while (isAlphaNum(LastChar = getChar()))
                Token += LastChar;

            if (Token == "def")
                return {TokenType::DEF_TOKEN};
            else if (Token == "extern")
                return {TokenType::EXTERN_TOKEN};
            else if (Token == "return")
                return {TokenType::RETURN_TOKEN};
            else if (Token == "end")
                return {TokenType::END_TOKEN};
            else if (Token == "var")
                return {TokenType::VAR_TOKEN};
            else if (Token == "if")
                return {TokenType::IF_TOKEN};
            else if (Token == "else")
                return {TokenType::ELSE_TOKEN};
            else if (Token == "do")
                return {TokenType::DO_TOKEN};
            else if (Token == "for")
                return {TokenType::FOR_TOKEN};
            else if (Token == "while")
                return {TokenType::WHILE_TOKEN};
            else if (Token == "import")
                return {TokenType::IMPORT_TOKEN};
            else if (Token == "of")
                return {TokenType::OF_TOKEN};
            else if (Types.find(Token) != Types.end()) {
                return {TokenType::TYPE, Token};
            } else if (Token == "true") {
                return {TokenType::BOOL, true};
            } else if (Token == "false") {
                return {TokenType::BOOL, false};
            } else
                return {TokenType::IDENTIFIER, Token};
        }

        if (LastChar == '"') {
            std::string Value = "";
            LastChar = getChar();
            while (LastChar != '"' && LastChar != EOF) {
                Value += LastChar;
                LastChar = getChar();
            }
            LastChar = getChar();
            return {TokenType::STRING, Value};
        }

        // Handle Digits
        if (isDigit(LastChar) || LastChar == '.') {
            bool decimal = false;
            std::string NumberString;

            do {
                if (LastChar == '.') {
                    if (decimal) {
                        LogError(&"Unexpected character: "[LastChar]);     //TODO: Errors and shit
                        return {TokenType::ERROR};
                    }
                    decimal = true;
                }
                NumberString += LastChar;
                LastChar = getChar();
            } while (isDigit(LastChar) || LastChar == '.');

            double Value = strtod(NumberString.c_str(), 0);
            return {TokenType::NUMBER, Value};
        }

        // Handle comments
        if (LastChar == '#') {
            do {
                LastChar = getChar();
            } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

            if (LastChar != EOF) {
                return getToken();
            }
        }

        if (Operators.find(std::string(1, LastChar)) != Operators.end()) {
            std::string op = std::string(1, LastChar);
            LastChar = getChar();
            if (Operators.find(op + std::string(1, LastChar)) != Operators.end()) {
                op += LastChar;
                LastChar = getChar();
            }
            return {TokenType::OPERATOR, op};
        }

        // Handle EOF
        if (LastChar == EOF) {
            return {TokenType::EOF_TOKEN};
        }

        // If something else; returns ascii value
        std::string returnValue = std::string(1, LastChar);
        LastChar = getChar();
        return {TokenType::UNDEFINED, returnValue};
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
}