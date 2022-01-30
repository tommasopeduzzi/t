//
// Created by tommasopeduzzi on 11/08/2021.
//

#pragma once

#include <string>
#include <fstream>
#include <set>
#include <variant>

namespace t {
    struct FileLocation {
        std::string file;
        int line;
        int column;
    };

    enum class TokenType {
        EOF_TOKEN,
        DEF_TOKEN,
        IMPORT_TOKEN,
        EXTERN_TOKEN,
        VAR_TOKEN,
        RETURN_TOKEN,
        IF_TOKEN,
        ELSE_TOKEN,
        FOR_TOKEN,
        WHILE_TOKEN,
        DO_TOKEN,
        END_TOKEN,
        OF_TOKEN,
        TYPE,
        OPERATOR,
        IDENTIFIER,
        NUMBER,
        STRING,
        BOOL,
        UNDEFINED,
        ERROR,
    };

    struct Token {
        TokenType type;
        std::variant<std::string, double, bool> value;
    };

    const std::set<std::string> Types{
            "number",
            "bool",
            "string",
            "void"
    };

    const std::set<std::string> Operators{
            "=",
            "+",
            "-",
            ">",
            "<",
            "->",
            ">=",
            "<=",
            "==",
    };

    class Lexer {
    public:
        Lexer(std::string filePath);

        char LastChar = ' ';
        std::ifstream file;
        FileLocation location;

        Token getToken();

        char getChar();

        bool isWhiteSpace(char c);

        bool isAlpha(char c);

        bool isAlphaNum(char c);

        bool isDigit(char c);
    };

    bool operator==(const Token &lhs, const char c);

    bool operator!=(const Token &lhs, const char c);

    const std::string digitRegex = "[0-9]";
    const std::string alphaNumRegex = "[a-zA-Z0-9]";
    const std::string alphaRegex = "[a-zA-Z]";
    const std::string whitespaceRegex = "\\s";

}