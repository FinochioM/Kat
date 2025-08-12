#include <iostream>
#include "Lexer.h"
#include "Parser.h"
#include "AST.h"

void printAST(Expression* expr, int depth = 0) {
    std::string indent(depth * 2, ' ');

    if (auto* num = dynamic_cast<NumberLiteral*>(expr)) {
        std::cout << indent << "Number: " << num->value << std::endl;
    } else if (auto* binOp = dynamic_cast<BinaryOperation*>(expr)) {
        std::cout << indent << "BinaryOp: " << binOp->operator_token << std::endl;
        std::cout << indent << "Left: " << std::endl;
        printAST(binOp->left.get(), depth + 1);
        std::cout << indent << "Right: " << std::endl;
        printAST(binOp->right.get(), depth +1);
    }
}

int main() {
    std::string source = "123 + 456 - 789";

    std::cout << "Parsing: \"" << source << "\"\n\n";

    Lexer lexer(source);
    Parser parser(lexer);

    try {
        auto ast = parser.parse();
        std::cout << "AST Structure:\n";
        printAST(ast.get());
    } catch (const std::exception& e) {
        std::cout << "Parse error: " << e.what() << std::endl;
    }

    return 0;
}