#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"

const char *token_type_to_str(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_CHAR: return "CHAR";
        case TOKEN_OPERATOR: return "OPERATOR";
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_PUNCTUATION: return "PUNCTUATION";
        case TOKEN_COMMENT: return "COMMENT";
        case TOKEN_UNKNOWN: return "UNKNOWN";
        default: return "?";
    }
}

const char *ast_type_to_str(ASTNodeType type) {
    switch (type) {
        case AST_PROGRAM: return "PROGRAM";
        case AST_PACKAGE: return "PACKAGE";
        case AST_IMPORT: return "IMPORT";
        case AST_PROC: return "PROC";
        case AST_BLOCK: return "BLOCK";
        case AST_VAR_DECL: return "VAR_DECL";
        case AST_ASSIGN: return "ASSIGN";
        case AST_IF: return "IF";
        case AST_FOR: return "FOR";
        case AST_RETURN: return "RETURN";
        case AST_CALL: return "CALL";
        case AST_BINARY: return "BINARY";
        case AST_UNARY: return "UNARY";
        case AST_LITERAL: return "LITERAL";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_STRUCT: return "STRUCT";
        case AST_ENUM: return "ENUM";
        case AST_UNKNOWN: return "UNKNOWN";
        default: return "?";
    }
}

void print_ast(const ASTNode *node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; ++i) printf("  ");
    printf("%s", ast_type_to_str(node->type));
    if (node->type == AST_IDENTIFIER || node->type == AST_LITERAL || node->type == AST_PACKAGE || node->type == AST_IMPORT) {
        printf(" ('%.*s')", (int)node->token.length, node->token.lexeme);
    }
    printf("\n");
    for (size_t i = 0; i < node->child_count; ++i) {
        print_ast(node->children[i], indent + 1);
    }
    if (node->left) {
        for (int i = 0; i < indent + 1; ++i) printf("  ");
        printf("left:\n");
        print_ast(node->left, indent + 2);
    }
    if (node->right) {
        for (int i = 0; i < indent + 1; ++i) printf("  ");
        printf("right:\n");
        print_ast(node->right, indent + 2);
    }
    if (node->cond) {
        for (int i = 0; i < indent + 1; ++i) printf("  ");
        printf("cond:\n");
        print_ast(node->cond, indent + 2);
    }
    if (node->body) {
        for (int i = 0; i < indent + 1; ++i) printf("  ");
        printf("body:\n");
        print_ast(node->body, indent + 2);
    }
    if (node->else_body) {
        for (int i = 0; i < indent + 1; ++i) printf("  ");
        printf("else:\n");
        print_ast(node->else_body, indent + 2);
    }
    if (node->arg_count > 0 && node->args) {
        for (int i = 0; i < indent + 1; ++i) printf("  ");
        printf("args:\n");
        for (size_t i = 0; i < node->arg_count; ++i) {
            print_ast(node->args[i], indent + 2);
        }
    }
}

int main(int argc, char **argv) {
    printf("Welcome to KatLanguage\n");
    const char *source =
        "package main\n"
        "import \"core:fmt\"\n"
        "proc :: main() {\n"
        "    x := 42\n"
        "    x := 100\n"
        "    y = 10\n"
        "    z := 5\n"
        "    z = 7\n"
        "    if z > 0 {\n"
        "        z := 1\n"
        "        z := 2\n"
        "        a = 3\n"
        "    }\n"
        "}\n"
        "proc :: main() {\n"
        "    b := 1\n"
        "}\n";
    Lexer lexer;
    lexer_init(&lexer, source);
    Parser parser;
    parser_init(&parser, &lexer);
    ASTNode *ast = parse_program(&parser);
    printf("\nAST:\n");
    print_ast(ast, 0);
    semantic_check(ast);
    Proto *proto = codegen_generate(ast);
    codegen_print(proto);
    codegen_free(proto);
    free_ast(ast);
    return 0;
} 