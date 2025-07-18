#ifndef KATLANGUAGE_PARSER_H
#define KATLANGUAGE_PARSER_H

#include "lexer.h"
#include <stddef.h>

typedef enum {
    AST_PROGRAM,
    AST_PACKAGE,
    AST_IMPORT,
    AST_PROC,
    AST_BLOCK,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_IF,
    AST_FOR,
    AST_RETURN,
    AST_CALL,
    AST_BINARY,
    AST_UNARY,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_STRUCT,
    AST_ENUM,
    AST_UNKNOWN
} ASTNodeType;

struct ASTNode;

typedef struct ASTNode {
    ASTNodeType type;
    struct ASTNode **children;
    size_t child_count;
    Token token;
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode **args;
    size_t arg_count;
    struct ASTNode *cond;
    struct ASTNode *body;
    struct ASTNode *else_body;
} ASTNode;

typedef struct {
    Lexer *lexer;
    Token current;
} Parser;

void parser_init(Parser *parser, Lexer *lexer);
ASTNode *parse_program(Parser *parser);
void free_ast(ASTNode *node);

#endif 