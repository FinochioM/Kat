#include "Parser.h"
#include <iostream>
#include <stdexcept>

Parser::Parser(Lexer& lexer) : lexer(lexer), currentToken(TokenType::UNKNOWN, "", 1, 1) {
    advance();
}

void Parser::advance() {
    currentToken = lexer.nextToken();
}

void Parser::expect(TokenType type) {
    if (currentToken.type != type) {
        throw std::runtime_error("Unexpected token: expected " +
                                std::to_string(static_cast<int>(type)) +
                                " but got " + currentToken.value);
    }
    advance();
}

std::unique_ptr<Expression> Parser::parse() {
    return parseExpression();
}

std::unique_ptr<Expression> Parser::parseExpression() {
    auto left = parseTerm();

    while (currentToken.type == TokenType::PLUS || currentToken.type == TokenType::MINUS) {
        std::string op = currentToken.value;
        advance();
        auto right = parseTerm();
        left = std::make_unique<BinaryOperation>(std::move(left), op, std::move(right));
    }

    return left;
}

std::unique_ptr<Expression> Parser::parseTerm() {
    return parseFactor();
}

std::unique_ptr<Expression> Parser::parseFactor() {
    if (currentToken.type == TokenType::NUMBER) {
        int value = std::stoi(currentToken.value);
        advance();
        return std::make_unique<NumberLiteral>(value);
    }

    throw std::runtime_error("Expected number, got: " + currentToken.value);
}