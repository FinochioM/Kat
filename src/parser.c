#include "parser.h"
#include <stdlib.h>
#include <string.h>

static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = 1;
    
    fprintf(stderr, "[line %d] Error", token->line);
    
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_INVALID) {
        
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    
    fprintf(stderr, ": %s\n", message);
    parser->had_error = 1;
}

static void error(Parser* parser, const char* message) {
    error_at(parser, &parser->previous, message);
}

static void error_at_current(Parser* parser, const char* message) {
    error_at(parser, &parser->current, message);
}

static void advance(Parser* parser) {
    parser->previous = parser->current;
    
    while (1) {
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type != TOKEN_INVALID) break;
        
        error_at_current(parser, "Unexpected character");
    }
}

static int check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static int match(Parser* parser, TokenType type) {
    if (!check(parser, type)) return 0;
    advance(parser);
    return 1;
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }
    
    error_at_current(parser, message);
}

static void synchronize(Parser* parser) {
    parser->panic_mode = 0;
    
    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_SEMICOLON) return;
        
        switch (parser->current.type) {
            case TOKEN_PROC:
            case TOKEN_STRUCT:
            case TOKEN_ENUM:
            case TOKEN_UNION:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
                return;
            default:
                break;
        }
        
        advance(parser);
    }
}

ASTNode* create_node(ASTNodeType type, int line, int column) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    node->type = type;
    node->line = line;
    node->column = column;
    
    return node;
}

ASTNodeList* create_node_list(void) {
    ASTNodeList* list = malloc(sizeof(ASTNodeList));
    if (!list) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    list->nodes = NULL;
    list->count = 0;
    list->capacity = 0;
    
    return list;
}

void add_node_to_list(ASTNodeList* list, ASTNode* node) {
    if (list->count >= list->capacity) {
        int new_capacity = list->capacity < 8 ? 8 : list->capacity * 2;
        ASTNode** new_nodes = realloc(list->nodes, new_capacity * sizeof(ASTNode*));
        if (!new_nodes) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        list->nodes = new_nodes;
        list->capacity = new_capacity;
    }
    
    list->nodes[list->count++] = node;
}

static ASTNode* declaration(Parser* parser);
static ASTNode* statement(Parser* parser);
static ASTNode* expression(Parser* parser);

static ASTNode* proc_declaration(Parser* parser) {
    consume(parser, TOKEN_IDENTIFIER, "Expected procedure name");
    
    char* name = malloc(parser->previous.length + 1);
    strncpy(name, parser->previous.start, parser->previous.length);
    name[parser->previous.length] = '\0';
    
    ASTNode* node = create_node(AST_PROC_DECL, parser->previous.line, parser->previous.column);
    node->proc_decl.name = name;
    node->proc_decl.params = create_node_list();
    node->proc_decl.return_type = NULL;
    
    consume(parser, TOKEN_LPAREN, "Expected '(' after procedure name");
    
    if (!check(parser, TOKEN_RPAREN)) {
        do {
            consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
            
            char* param_name = malloc(parser->previous.length + 1);
            strncpy(param_name, parser->previous.start, parser->previous.length);
            param_name[parser->previous.length] = '\0';
            
            ASTNode* param = create_node(AST_PARAM_DECL, parser->previous.line, parser->previous.column);
            param->param_decl.name = param_name;
            param->param_decl.type = NULL;
            
            if (match(parser, TOKEN_COLON)) {
                consume(parser, TOKEN_IDENTIFIER, "Expected type name");
                
                char* type_name = malloc(parser->previous.length + 1);
                strncpy(type_name, parser->previous.start, parser->previous.length);
                type_name[parser->previous.length] = '\0';
                
                param->param_decl.type = create_node(AST_TYPE_EXPR, parser->previous.line, parser->previous.column);
                param->param_decl.type->type_expr.name = type_name;
            }
            
            add_node_to_list(node->proc_decl.params, param);
        } while (match(parser, TOKEN_COMMA));
    }
    
    consume(parser, TOKEN_RPAREN, "Expected ')' after parameters");
    
    if (match(parser, TOKEN_ARROW)) {
        consume(parser, TOKEN_IDENTIFIER, "Expected return type");
        
        char* return_type = malloc(parser->previous.length + 1);
        strncpy(return_type, parser->previous.start, parser->previous.length);
        return_type[parser->previous.length] = '\0';
        
        node->proc_decl.return_type = create_node(AST_TYPE_EXPR, parser->previous.line, parser->previous.column);
        node->proc_decl.return_type->type_expr.name = return_type;
    }
    
    node->proc_decl.body = statement(parser);
    
    return node;
}

static ASTNode* var_declaration(Parser* parser) {
    if (!check(parser, TOKEN_IDENTIFIER)) {
        error_at_current(parser, "Expected variable name");
        return NULL;
    }
    
    char* name = malloc(parser->current.length + 1);
    strncpy(name, parser->current.start, parser->current.length);
    name[parser->current.length] = '\0';
    
    ASTNode* node = create_node(AST_VAR_DECL, parser->current.line, parser->current.column);
    node->var_decl.name = name;
    node->var_decl.type = NULL;
    node->var_decl.value = NULL;
    
    advance(parser);
    
    if (match(parser, TOKEN_COLON_EQUAL)) {
        node->var_decl.is_inferred = 1;
        node->var_decl.value = expression(parser);
    } else if (match(parser, TOKEN_COLON)) {
        node->var_decl.is_inferred = 0;
        
        consume(parser, TOKEN_IDENTIFIER, "Expected type name");
        
        char* type_name = malloc(parser->previous.length + 1);
        strncpy(type_name, parser->previous.start, parser->previous.length);
        type_name[parser->previous.length] = '\0';
        
        node->var_decl.type = create_node(AST_TYPE_EXPR, parser->previous.line, parser->previous.column);
        node->var_decl.type->type_expr.name = type_name;
        
        if (match(parser, TOKEN_EQUAL)) {
            node->var_decl.value = expression(parser);
        }
    } else {
        error(parser, "Expected ':' or ':=' after variable name");
    }
    
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration");
    
    return node;
}

static ASTNode* var_declaration_or_assignment(Parser* parser) {
    char* name = malloc(parser->current.length + 1);
    strncpy(name, parser->current.start, parser->current.length);
    name[parser->current.length] = '\0';
    
    advance(parser);
    
    if (match(parser, TOKEN_EQUAL)) {
        ASTNode* node = create_node(AST_ASSIGN_STMT, parser->previous.line, parser->previous.column);
        node->assign_stmt.name = name;
        node->assign_stmt.value = expression(parser);
        
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after assignment");
        return node;
    } else {
        ASTNode* node = create_node(AST_VAR_DECL, parser->previous.line, parser->previous.column);
        node->var_decl.name = name;
        node->var_decl.type = NULL;
        node->var_decl.value = NULL;
        
        if (match(parser, TOKEN_COLON_EQUAL)) {
            node->var_decl.is_inferred = 1;
            node->var_decl.value = expression(parser);
        } else if (match(parser, TOKEN_COLON)) {
            node->var_decl.is_inferred = 0;
            
            consume(parser, TOKEN_IDENTIFIER, "Expected type name");
            
            char* type_name = malloc(parser->previous.length + 1);
            strncpy(type_name, parser->previous.start, parser->previous.length);
            type_name[parser->previous.length] = '\0';
            
            node->var_decl.type = create_node(AST_TYPE_EXPR, parser->previous.line, parser->previous.column);
            node->var_decl.type->type_expr.name = type_name;
            
            if (match(parser, TOKEN_EQUAL)) {
                node->var_decl.value = expression(parser);
            }
        } else {
            error(parser, "Expected ':' or ':=' after variable name");
        }
        
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration");
        return node;
    }
}

static ASTNode* declaration(Parser* parser) {
    if (match(parser, TOKEN_PROC)) {
        return proc_declaration(parser);
    }
    
    return var_declaration(parser);
}

static ASTNode* block_statement(Parser* parser) {
    ASTNode* node = create_node(AST_BLOCK_STMT, parser->previous.line, parser->previous.column);
    node->block_stmt.statements = create_node_list();
    
    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_NEWLINE)) continue;
        
        ASTNode* stmt;
        
        if (check(parser, TOKEN_PROC) || check(parser, TOKEN_STRUCT) || check(parser, TOKEN_ENUM) || check(parser, TOKEN_UNION)) {
            stmt = declaration(parser);
        } else {
            stmt = statement(parser);
        }
        
        if (stmt) {
            add_node_to_list(node->block_stmt.statements, stmt);
        }
    }
    
    consume(parser, TOKEN_RBRACE, "Expected '}' after block");
    
    return node;
}

static ASTNode* if_statement(Parser* parser) {
    ASTNode* node = create_node(AST_IF_STMT, parser->previous.line, parser->previous.column);
    
    node->if_stmt.condition = expression(parser);
    node->if_stmt.then_stmt = statement(parser);
    node->if_stmt.else_stmt = NULL;
    
    if (match(parser, TOKEN_ELSE)) {
        node->if_stmt.else_stmt = statement(parser);
    }
    
    return node;
}

static ASTNode* while_statement(Parser* parser) {
    ASTNode* node = create_node(AST_WHILE_STMT, parser->previous.line, parser->previous.column);
    
    node->while_stmt.condition = expression(parser);
    node->while_stmt.body = statement(parser);
    
    return node;
}

static ASTNode* return_statement(Parser* parser) {
    ASTNode* node = create_node(AST_RETURN_STMT, parser->previous.line, parser->previous.column);
    
    if (!check(parser, TOKEN_SEMICOLON)) {
        node->return_stmt.value = expression(parser);
    } else {
        node->return_stmt.value = NULL;
    }
    
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after return value");
    
    return node;
}

static ASTNode* expression_statement(Parser* parser) {
    ASTNode* expr = expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression");
    
    ASTNode* node = create_node(AST_EXPR_STMT, parser->previous.line, parser->previous.column);
    node->expr_stmt.expression = expr;
    
    return node;
}

static ASTNode* statement(Parser* parser) {
    if (match(parser, TOKEN_LBRACE)) {
        return block_statement(parser);
    }
    
    if (match(parser, TOKEN_IF)) {
        return if_statement(parser);
    }
    
    if (match(parser, TOKEN_RETURN)) {
        return return_statement(parser);
    }
    
    if (match(parser, TOKEN_WHILE)) {
        return while_statement(parser);
    }
    
    if (check(parser, TOKEN_IDENTIFIER)) {
        Lexer saved_lexer = *parser->lexer;
        Token saved_current = parser->current;
        Token saved_previous = parser->previous;
        
        advance(parser);
        
        if (check(parser, TOKEN_COLON) || check(parser, TOKEN_COLON_EQUAL) || check(parser, TOKEN_EQUAL)) {
            *parser->lexer = saved_lexer;
            parser->current = saved_current;
            parser->previous = saved_previous;
            return var_declaration_or_assignment(parser);
        } else {
            *parser->lexer = saved_lexer;
            parser->current = saved_current;
            parser->previous = saved_previous;
            return expression_statement(parser);
        }
    }
    
    return expression_statement(parser);
}

static ASTNode* call_expression(Parser* parser, char* name) {
    ASTNode* node = create_node(AST_CALL_EXPR, parser->previous.line, parser->previous.column);
    node->call_expr.name = name;
    node->call_expr.args = create_node_list();
    
    if (!check(parser, TOKEN_RPAREN)) {
        do {
            ASTNode* arg = expression(parser);
            add_node_to_list(node->call_expr.args, arg);
        } while (match(parser, TOKEN_COMMA));
    }
    
    consume(parser, TOKEN_RPAREN, "Expected ')' after arguments");
    
    return node;
}

static ASTNode* primary_expression(Parser* parser) {
    if (match(parser, TOKEN_TRUE) || match(parser, TOKEN_FALSE)) {
        ASTNode* node = create_node(AST_LITERAL_EXPR, parser->previous.line, parser->previous.column);
        node->literal_expr.type = parser->previous.type;
        
        char* value = malloc(parser->previous.length + 1);
        strncpy(value, parser->previous.start, parser->previous.length);
        value[parser->previous.length] = '\0';
        node->literal_expr.value = value;
        
        return node;
    }
    
    if (match(parser, TOKEN_NIL)) {
        ASTNode* node = create_node(AST_LITERAL_EXPR, parser->previous.line, parser->previous.column);
        node->literal_expr.type = TOKEN_NIL;
        node->literal_expr.value = strdup("nil");
        return node;
    }
    
    if (match(parser, TOKEN_NUMBER)) {
        ASTNode* node = create_node(AST_LITERAL_EXPR, parser->previous.line, parser->previous.column);
        node->literal_expr.type = TOKEN_NUMBER;
        
        char* value = malloc(parser->previous.length + 1);
        strncpy(value, parser->previous.start, parser->previous.length);
        value[parser->previous.length] = '\0';
        node->literal_expr.value = value;
        
        return node;
    }
    
    if (match(parser, TOKEN_STRING)) {
        ASTNode* node = create_node(AST_LITERAL_EXPR, parser->previous.line, parser->previous.column);
        node->literal_expr.type = TOKEN_STRING;
        
        char* value = malloc(parser->previous.length + 1);
        strncpy(value, parser->previous.start, parser->previous.length);
        value[parser->previous.length] = '\0';
        node->literal_expr.value = value;
        
        return node;
    }
    
    if (match(parser, TOKEN_IDENTIFIER)) {
        char* name = malloc(parser->previous.length + 1);
        strncpy(name, parser->previous.start, parser->previous.length);
        name[parser->previous.length] = '\0';
        
        if (match(parser, TOKEN_LPAREN)) {
            return call_expression(parser, name);
        }
        
        ASTNode* node = create_node(AST_IDENTIFIER_EXPR, parser->previous.line, parser->previous.column);
        node->identifier_expr.name = name;
        
        return node;
    }
    
    if (match(parser, TOKEN_LPAREN)) {
        ASTNode* expr = expression(parser);
        consume(parser, TOKEN_RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    error_at_current(parser, "Expected expression");
    return NULL;
}

static ASTNode* factor_expression(Parser* parser) {
    ASTNode* expr = primary_expression(parser);
    
    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH)) {
        TokenType operator = parser->previous.type;
        ASTNode* right = primary_expression(parser);
        
        ASTNode* binary = create_node(AST_BINARY_EXPR, parser->previous.line, parser->previous.column);
        binary->binary_expr.left = expr;
        binary->binary_expr.operator = operator;
        binary->binary_expr.right = right;
        
        expr = binary;
    }
    
    return expr;
}

static ASTNode* term_expression(Parser* parser) {
    ASTNode* expr = factor_expression(parser);
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        TokenType operator = parser->previous.type;
        ASTNode* right = factor_expression(parser);
        
        ASTNode* binary = create_node(AST_BINARY_EXPR, parser->previous.line, parser->previous.column);
        binary->binary_expr.left = expr;
        binary->binary_expr.operator = operator;
        binary->binary_expr.right = right;
        
        expr = binary;
    }
    
    return expr;
}

static ASTNode* comparison_expression(Parser* parser) {
    ASTNode* expr = term_expression(parser);
    
    while (match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQUAL) ||
           match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQUAL)) {
        TokenType operator = parser->previous.type;
        ASTNode* right = term_expression(parser);
        
        ASTNode* binary = create_node(AST_BINARY_EXPR, parser->previous.line, parser->previous.column);
        binary->binary_expr.left = expr;
        binary->binary_expr.operator = operator;
        binary->binary_expr.right = right;
        
        expr = binary;
    }
    
    return expr;
}

static ASTNode* expression(Parser* parser) {
    return comparison_expression(parser);
}

void parser_init(Parser* parser, Lexer* lexer) {
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    
    advance(parser);
}

ASTNode* parse_program(Parser* parser) {
    ASTNode* program = create_node(AST_PROGRAM, 1, 1);
    program->program.declarations = create_node_list();
    
    while (!check(parser, TOKEN_EOF)) {
        if (match(parser, TOKEN_NEWLINE)) continue;
        
        ASTNode* decl = declaration(parser);
        if (decl) {
            add_node_to_list(program->program.declarations, decl);
        }
        
        if (parser->panic_mode) synchronize(parser);
    }
    
    return program;
}

void print_ast(ASTNode* node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program\n");
            for (int i = 0; i < node->program.declarations->count; i++) {
                print_ast(node->program.declarations->nodes[i], indent + 1);
            }
            break;
        case AST_PROC_DECL:
            printf("ProcDecl: %s\n", node->proc_decl.name);
            if (node->proc_decl.body) {
                print_ast(node->proc_decl.body, indent + 1);
            }
            break;
        case AST_VAR_DECL:
            printf("VarDecl: %s %s\n", node->var_decl.name, 
                   node->var_decl.is_inferred ? "(inferred)" : "(explicit)");
            if (node->var_decl.value) {
                print_ast(node->var_decl.value, indent + 1);
            }
            break;
        case AST_ASSIGN_STMT:
            printf("Assignment: %s\n", node->assign_stmt.name);
            print_ast(node->assign_stmt.value, indent + 1);
            break;
        case AST_BLOCK_STMT:
            printf("Block\n");
            for (int i = 0; i < node->block_stmt.statements->count; i++) {
                print_ast(node->block_stmt.statements->nodes[i], indent + 1);
            }
            break;
        case AST_IF_STMT:
            printf("If\n");
            print_ast(node->if_stmt.condition, indent + 1);
            print_ast(node->if_stmt.then_stmt, indent + 1);
            if (node->if_stmt.else_stmt) {
                print_ast(node->if_stmt.else_stmt, indent + 1);
            }
            break;
        case AST_WHILE_STMT:
            printf("While\n");
            print_ast(node->while_stmt.condition, indent + 1);
            print_ast(node->while_stmt.body, indent + 1);
            break;
        case AST_RETURN_STMT:
            printf("Return\n");
            if (node->return_stmt.value) {
                print_ast(node->return_stmt.value, indent + 1);
            }
            break;
        case AST_EXPR_STMT:
            printf("ExprStmt\n");
            print_ast(node->expr_stmt.expression, indent + 1);
            break;
        case AST_CALL_EXPR:
            printf("Call: %s\n", node->call_expr.name);
            for (int i = 0; i < node->call_expr.args->count; i++) {
                print_ast(node->call_expr.args->nodes[i], indent + 1);
            }
            break;
        case AST_BINARY_EXPR:
            printf("Binary: %s\n", token_type_to_string(node->binary_expr.operator));
            print_ast(node->binary_expr.left, indent + 1);
            print_ast(node->binary_expr.right, indent + 1);
            break;
        case AST_LITERAL_EXPR:
            printf("Literal: %s\n", node->literal_expr.value);
            break;
        case AST_IDENTIFIER_EXPR:
            printf("Identifier: %s\n", node->identifier_expr.name);
            break;
        default:
            printf("Unknown node type\n");
            break;
    }
}

void free_ast(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            if (node->program.declarations) {
                for (int i = 0; i < node->program.declarations->count; i++) {
                    free_ast(node->program.declarations->nodes[i]);
                }
                free(node->program.declarations->nodes);
                free(node->program.declarations);
            }
            break;
        case AST_PROC_DECL:
            free(node->proc_decl.name);
            if (node->proc_decl.params) {
                for (int i = 0; i < node->proc_decl.params->count; i++) {
                    free_ast(node->proc_decl.params->nodes[i]);
                }
                free(node->proc_decl.params->nodes);
                free(node->proc_decl.params);
            }
            free_ast(node->proc_decl.return_type);
            free_ast(node->proc_decl.body);
            break;
        case AST_VAR_DECL:
            free(node->var_decl.name);
            free_ast(node->var_decl.type);
            free_ast(node->var_decl.value);
            break;
        case AST_ASSIGN_STMT:
            free(node->assign_stmt.name);
            free_ast(node->assign_stmt.value);
            break;
        case AST_BLOCK_STMT:
            if (node->block_stmt.statements) {
                for (int i = 0; i < node->block_stmt.statements->count; i++) {
                    free_ast(node->block_stmt.statements->nodes[i]);
                }
                free(node->block_stmt.statements->nodes);
                free(node->block_stmt.statements);
            }
            break;
        case AST_IF_STMT:
            free_ast(node->if_stmt.condition);
            free_ast(node->if_stmt.then_stmt);
            free_ast(node->if_stmt.else_stmt);
            break;
        case AST_WHILE_STMT:
            free_ast(node->while_stmt.condition);
            free_ast(node->while_stmt.body);
            break;
        case AST_RETURN_STMT:
            free_ast(node->return_stmt.value);
            break;
        case AST_EXPR_STMT:
            free_ast(node->expr_stmt.expression);
            break;
        case AST_CALL_EXPR:
            free(node->call_expr.name);
            if (node->call_expr.args) {
                for (int i = 0; i < node->call_expr.args->count; i++) {
                    free_ast(node->call_expr.args->nodes[i]);
                }
                free(node->call_expr.args->nodes);
                free(node->call_expr.args);
            }
            break;
        case AST_BINARY_EXPR:
            free_ast(node->binary_expr.left);
            free_ast(node->binary_expr.right);
            break;
        case AST_LITERAL_EXPR:
            free(node->literal_expr.value);
            break;
        case AST_IDENTIFIER_EXPR:
            free(node->identifier_expr.name);
            break;
        default:
            break;
    }
    
    free(node);
}

void free_node_list(ASTNodeList* list) {
    if (!list) return;
    
    for (int i = 0; i < list->count; i++) {
        free_ast(list->nodes[i]);
    }
    
    free(list->nodes);
    free(list);
}