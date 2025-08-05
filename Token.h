#pragma once
#include <string>

enum class TokenType {
    NUMBER,
    IDENTIFIER,

    PLUS,
    MINUS,

    EOF_TOKEN,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token(TokenType t, const std::string& v, int l = 1, int c = 1) : type(t), value(v), line(l), column(c) {}
};