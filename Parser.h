#pragma once
#include "Token.h"
#include "Lexer.h"
#include "AST.h"
#include <memory>

class Parser {
private:
    Lexer& lexer;
    Token currentToken;

    void advance();
    void expect(TokenType type);

    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseTerm();
    std::unique_ptr<Expression> parseFactor();

public:
    Parser(Lexer& lexer);
    std::unique_ptr<Expression> parse();
};