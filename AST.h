#pragma once
#include <memory>
#include <string>

class ASTNode {
public:
    virtual ~ASTNode() = default;
};

class Expression : public ASTNode {
public:
    virtual ~Expression() = default;
};

class NumberLiteral : public Expression {
public:
    int value;

    NumberLiteral(int val) : value(val) {}
};

class BinaryOperation : public Expression {
public:
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    std::string operator_token;

    BinaryOperation(std::unique_ptr<Expression> l,
                    std::string op,
                    strd::unique_ptr<Expression> r)
                : left(std::move(l)), operator_token(op), right(std::move(r)) {}
};