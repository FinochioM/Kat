#include <iostream>
#include "Lexer.h"
#include "Token.h"

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::EOF_TOKEN: return "EOF";
        case TokenType::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

int main() {
    std::string source = "123 + hello - 456 world";

    std::cout << "Tokenizando: \"" << source << "\"\n\n";

    Lexer lexer(source);

    while (true) {
        Token token = lexer.nextToken();

        std::cout << "Token: " << tokenTypeToString(token.type)
                  << " | Value: \"" << token.value << "\" "
                  << " | Line: " << token.line
                  << " | Column: " << token.column << std::endl;

        if (token.type == TokenType::EOF_TOKEN) {
            break;
        }
    }

    return 0;
}