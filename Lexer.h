#pragma once
#include "Token.h"
#include <string>

class Lexer {
private:
    std::string source;
    size_t current;
    int line;
    int column;

    char peek();
    char advance();
    bool isAtEnd();
    bool isDigit(char c);
    bool isAlpha(char c);
    bool isAlphanumeric(char c);

    Token number();
    Token identifier();
    void skipWhitespace();

public:
    Lexer(const std::string& source);
    Token nextToken();
};