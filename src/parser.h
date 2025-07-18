#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// AST Node types
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

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct ASTNodeList ASTNodeList;

// Node list for storing multiple nodes
struct ASTNodeList {
    ASTNode** nodes;
    int count;
    int capacity;
};

// Main AST node structure
struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    
    union {
        // Program node
        struct {
            ASTNodeList* declarations;
        } program;
        
        // Procedure declaration
        struct {
            char* name;
            ASTNodeList* params;
            ASTNode* return_type;
            ASTNode* body;
        } proc_decl;
        
        // Variable declaration
        struct {
            char* name;
            ASTNode* type;
            ASTNode* value;
            int is_inferred; // true if using :=
        } var_decl;
        
        // Assignment statement
        struct {
            char* name;
            ASTNode* value;
        } assign_stmt;
        
        // If statement
        struct {
            ASTNode* condition;
            ASTNode* then_stmt;
            ASTNode* else_stmt;
        } if_stmt;
        
        // While statement
        struct {
            ASTNode* condition;
            ASTNode* body;
        } while_stmt;
        
        // For statement
        struct {
            ASTNode* init;
            ASTNode* condition;
            ASTNode* update;
            ASTNode* body;
        } for_stmt;
        
        // Return statement
        struct {
            ASTNode* value;
        } return_stmt;
        
        // Block statement
        struct {
            ASTNodeList* statements;
        } block_stmt;
        
        // Expression statement
        struct {
            ASTNode* expression;
        } expr_stmt;
        
        // Call expression
        struct {
            char* name;
            ASTNodeList* args;
        } call_expr;
        
        // Binary expression
        struct {
            ASTNode* left;
            TokenType operator;
            ASTNode* right;
        } binary_expr;
        
        // Unary expression
        struct {
            TokenType operator;
            ASTNode* operand;
        } unary_expr;
        
        // Literal expression
        struct {
            TokenType type;
            char* value;
        } literal_expr;
        
        // Identifier expression
        struct {
            char* name;
        } identifier_expr;
        
        // Struct declaration
        struct {
            char* name;
            ASTNodeList* fields;
        } struct_decl;
        
        // Field declaration
        struct {
            char* name;
            ASTNode* type;
        } field_decl;
        
        // Parameter declaration
        struct {
            char* name;
            ASTNode* type;
        } param_decl;
        
        // Type expression
        struct {
            char* name;
        } type_expr;
    };
};

// Parser structure
typedef struct {
    Lexer* lexer;
    Token current;
    Token previous;
    int had_error;
    int panic_mode;
} Parser;

// Function declarations
void parser_init(Parser* parser, Lexer* lexer);
ASTNode* parse_program(Parser* parser);
void print_ast(ASTNode* node, int indent);
void free_ast(ASTNode* node);

// Helper functions
ASTNode* create_node(ASTNodeType type, int line, int column);
ASTNodeList* create_node_list(void);
void add_node_to_list(ASTNodeList* list, ASTNode* node);
void free_node_list(ASTNodeList* list);

#endif // PARSER_H