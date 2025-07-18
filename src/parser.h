#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_PROC_DECL,
    AST_VAR_DECL,
    AST_ASSIGN_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_BLOCK_STMT,
    AST_EXPR_STMT,
    AST_CALL_EXPR,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_LITERAL_EXPR,
    AST_IDENTIFIER_EXPR,
    AST_STRUCT_DECL,
    AST_ENUM_DECL,
    AST_UNION_DECL,
    AST_FIELD_DECL,
    AST_PARAM_DECL,
    AST_TYPE_EXPR
} ASTNodeType;

typedef struct ASTNode ASTNode;
typedef struct ASTNodeList ASTNodeList;

struct ASTNodeList {
    ASTNode** nodes;
    int count;
    int capacity;
};

struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    
    union {
        struct {
            ASTNodeList* declarations;
        } program;
        
        struct {
            char* name;
            ASTNodeList* params;
            ASTNode* return_type;
            ASTNode* body;
        } proc_decl;
        
        struct {
            char* name;
            ASTNode* type;
            ASTNode* value;
            int is_inferred;
        } var_decl;
        
        struct {
            char* name;
            ASTNode* value;
        } assign_stmt;
        
        struct {
            ASTNode* condition;
            ASTNode* then_stmt;
            ASTNode* else_stmt;
        } if_stmt;
        
        struct {
            ASTNode* condition;
            ASTNode* body;
        } while_stmt;
        
        struct {
            ASTNode* init;
            ASTNode* condition;
            ASTNode* update;
            ASTNode* body;
        } for_stmt;
        
        struct {
            ASTNode* value;
        } return_stmt;
        
        struct {
            ASTNodeList* statements;
        } block_stmt;
        
        struct {
            ASTNode* expression;
        } expr_stmt;
        
        struct {
            char* name;
            ASTNodeList* args;
        } call_expr;
        
        struct {
            ASTNode* left;
            TokenType operator;
            ASTNode* right;
        } binary_expr;
        
        struct {
            TokenType operator;
            ASTNode* operand;
        } unary_expr;
        
        struct {
            TokenType type;
            char* value;
        } literal_expr;
        
        struct {
            char* name;
        } identifier_expr;
        
        struct {
            char* name;
            ASTNodeList* fields;
        } struct_decl;
        
        struct {
            char* name;
            ASTNode* type;
        } field_decl;
        
        struct {
            char* name;
            ASTNode* type;
        } param_decl;
        
        struct {
            char* name;
        } type_expr;
    };
};

typedef struct {
    Lexer* lexer;
    Token current;
    Token previous;
    int had_error;
    int panic_mode;
} Parser;

void parser_init(Parser* parser, Lexer* lexer);
ASTNode* parse_program(Parser* parser);
void print_ast(ASTNode* node, int indent);
void free_ast(ASTNode* node);

ASTNode* create_node(ASTNodeType type, int line, int column);
ASTNodeList* create_node_list(void);
void add_node_to_list(ASTNodeList* list, ASTNode* node);
void free_node_list(ASTNodeList* list);

#endif