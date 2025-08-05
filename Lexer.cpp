#include "Lexer.h"
#include <cctype>

Lexer::Lexer(const std::string& source) : source(source), current(0), line(1), column(1) {}

char Lexer::peek() {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = source[current++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }

    return c;
}

bool Lexer::isAtEnd() {
    return current >= source.length();
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphanumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

Token Lexer::number() {
    std::string value;
    while (isDigit(peek())) {
        value += advance();
    }

    return Token(TokenType::NUMBER, value, line, column);
}

Token Lexer::identifier() {
    std::string value;
    while(isAlphanumeric(peek())) {
        value += advance();
    }

    return Token(TokenType::IDENTIFIER, value, line, column);
}

Token Lexer::nextToken() {
    skipWhitespace();

    if (isAtEnd()) {
        return Token(TokenType::EOF_TOKEN, "", line, column);
    }

    char c = advance();

    switch(c) {
        case '+': return Token(TokenType::PLUS, "+", line, column);
        case '-': return Token(TokenType::MINUS, "-", line, column);
        default:
        if (isDigit(c)) {
            current--;
            column--;
            return number();
        }
        if (isAlpha(c)) {
            current--;
            column--;
            return identifier();
        }

        return Token(TokenType::UNKNOWN, std::string(1, c), line, column);
    }
}