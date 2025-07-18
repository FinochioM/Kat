#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static ASTNode *new_ast_node(ASTNodeType type, Token token) {
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = type;
    node->children = NULL;
    node->child_count = 0;
    node->token = token;
    return node;
}

static void add_child(ASTNode *parent, ASTNode *child) {
    parent->children = (ASTNode **)realloc(parent->children, sizeof(ASTNode *) * (parent->child_count + 1));
    parent->children[parent->child_count++] = child;
}

static void parser_advance(Parser *parser) {
    do {
        parser->current = lexer_next_token(parser->lexer);
    } while (parser->current.type == TOKEN_COMMENT);
}

void parser_init(Parser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser_advance(parser);
}

static ASTNode *parse_package(Parser *parser);
static ASTNode *parse_import(Parser *parser);
static ASTNode *parse_proc(Parser *parser);
static ASTNode *parse_block(Parser *parser);
static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_if(Parser *parser);
static ASTNode *parse_for(Parser *parser);
static ASTNode *parse_return(Parser *parser);
static ASTNode *parse_var_decl_or_assign(Parser *parser);
static ASTNode *parse_expression(Parser *parser, int min_prec);
static ASTNode *parse_primary(Parser *parser);
static ASTNode *parse_call_or_identifier(Parser *parser);
static int get_operator_precedence(const Token *tok);
static ASTNode *parse_unknown(Parser *parser);

ASTNode *parse_program(Parser *parser) {
    ASTNode *root = new_ast_node(AST_PROGRAM, parser->current);
    while (parser->current.type != TOKEN_EOF) {
        if (parser->current.type == TOKEN_KEYWORD && strncmp(parser->current.lexeme, "package", parser->current.length) == 0) {
            add_child(root, parse_package(parser));
        } else if (parser->current.type == TOKEN_KEYWORD && strncmp(parser->current.lexeme, "import", parser->current.length) == 0) {
            add_child(root, parse_import(parser));
        } else if (parser->current.type == TOKEN_KEYWORD && strncmp(parser->current.lexeme, "proc", parser->current.length) == 0) {
            add_child(root, parse_proc(parser));
        } else if (parser->current.type == TOKEN_COMMENT) {
            parser_advance(parser);
        } else {
            add_child(root, parse_unknown(parser));
        }
    }
    return root;
}

static ASTNode *parse_package(Parser *parser) {
    ASTNode *node = new_ast_node(AST_PACKAGE, parser->current);
    parser_advance(parser);
    if (parser->current.type == TOKEN_IDENTIFIER) {
        add_child(node, new_ast_node(AST_IDENTIFIER, parser->current));
        parser_advance(parser);
    }
    return node;
}

static ASTNode *parse_import(Parser *parser) {
    ASTNode *node = new_ast_node(AST_IMPORT, parser->current);
    parser_advance(parser);
    if (parser->current.type == TOKEN_STRING) {
        add_child(node, new_ast_node(AST_LITERAL, parser->current));
        parser_advance(parser);
    }
    return node;
}

static ASTNode *parse_proc(Parser *parser) {
    ASTNode *node = new_ast_node(AST_PROC, parser->current);
    parser_advance(parser);
    if (parser->current.type == TOKEN_OPERATOR && parser->current.length == 2 && strncmp(parser->current.lexeme, "::", 2) == 0) {
        parser_advance(parser);
    }
    if (parser->current.type == TOKEN_IDENTIFIER) {
        add_child(node, new_ast_node(AST_IDENTIFIER, parser->current));
        parser_advance(parser);
    }
    if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == '(') {
        while (parser->current.type != TOKEN_PUNCTUATION || parser->current.lexeme[0] != ')') {
            if (parser->current.type == TOKEN_EOF) return node;
            parser_advance(parser);
        }
        parser_advance(parser);
    }
    if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == '{') {
        add_child(node, parse_block(parser));
    }
    return node;
}

static ASTNode *parse_block(Parser *parser) {
    ASTNode *node = new_ast_node(AST_BLOCK, parser->current);
    parser_advance(parser);
    while (parser->current.type != TOKEN_PUNCTUATION || parser->current.lexeme[0] != '}') {
        if (parser->current.type == TOKEN_EOF) return node;
        if (parser->current.type == TOKEN_COMMENT) {
            parser_advance(parser);
            continue;
        }
        ASTNode *stmt = parse_statement(parser);
        if (stmt) add_child(node, stmt);
    }
    parser_advance(parser);
    return node;
}

static ASTNode *parse_statement(Parser *parser) {
    if (parser->current.type == TOKEN_COMMENT) {
        parser_advance(parser);
        return NULL;
    }
    if (parser->current.type == TOKEN_KEYWORD && strncmp(parser->current.lexeme, "if", parser->current.length) == 0) {
        return parse_if(parser);
    } else if (parser->current.type == TOKEN_KEYWORD && strncmp(parser->current.lexeme, "for", parser->current.length) == 0) {
        return parse_for(parser);
    } else if (parser->current.type == TOKEN_KEYWORD && strncmp(parser->current.lexeme, "return", parser->current.length) == 0) {
        return parse_return(parser);
    } else if (parser->current.type == TOKEN_IDENTIFIER) {
        return parse_var_decl_or_assign(parser);
    } else if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == ';') {
        parser_advance(parser);
        return NULL;
    } else {
        parser_advance(parser);
        return NULL;
    }
}

static ASTNode *parse_if(Parser *parser) {
    ASTNode *node = new_ast_node(AST_IF, parser->current);
    parser_advance(parser);
    node->cond = parse_expression(parser, 0);
    if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == '{') {
        node->body = parse_block(parser);
    }
    if (parser->current.type == TOKEN_KEYWORD && strncmp(parser->current.lexeme, "else", parser->current.length) == 0) {
        parser_advance(parser);
        if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == '{') {
            node->else_body = parse_block(parser);
        }
    }
    return node;
}

static ASTNode *parse_for(Parser *parser) {
    ASTNode *node = new_ast_node(AST_FOR, parser->current);
    parser_advance(parser);
    node->cond = parse_expression(parser, 0);
    if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == '{') {
        node->body = parse_block(parser);
    }
    return node;
}

static ASTNode *parse_return(Parser *parser) {
    ASTNode *node = new_ast_node(AST_RETURN, parser->current);
    parser_advance(parser);
    if (parser->current.type != TOKEN_PUNCTUATION || parser->current.lexeme[0] != ';') {
        node->left = parse_expression(parser, 0);
    }
    if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == ';') {
        parser_advance(parser);
    }
    return node;
}

static ASTNode *parse_var_decl_or_assign(Parser *parser) {
    ASTNode *id = new_ast_node(AST_IDENTIFIER, parser->current);
    parser_advance(parser);
    if (parser->current.type == TOKEN_OPERATOR && parser->current.length == 2 && strncmp(parser->current.lexeme, ":=", 2) == 0) {
        ASTNode *node = new_ast_node(AST_VAR_DECL, parser->current);
        parser_advance(parser);
        node->left = id;
        node->right = parse_expression(parser, 0);
        if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == ';') {
            parser_advance(parser);
        }
        return node;
    } else if (parser->current.type == TOKEN_OPERATOR && parser->current.length == 1 && parser->current.lexeme[0] == '=') {
        ASTNode *node = new_ast_node(AST_ASSIGN, parser->current);
        parser_advance(parser);
        node->left = id;
        node->right = parse_expression(parser, 0);
        if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == ';') {
            parser_advance(parser);
        }
        return node;
    } else if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == '(') {
        ASTNode *expr = id;
        while (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == '(') {
            ASTNode *call = new_ast_node(AST_CALL, parser->current);
            call->left = expr;
            parser_advance(parser);
            while (parser->current.type != TOKEN_PUNCTUATION || parser->current.lexeme[0] != ')') {
                if (parser->current.type == TOKEN_EOF) break;
                ASTNode *arg = parse_expression(parser, 0);
                call->args = (ASTNode **)realloc(call->args, sizeof(ASTNode *) * (call->arg_count + 1));
                call->args[call->arg_count++] = arg;
                if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == ',') {
                    parser_advance(parser);
                }
            }
            if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == ')') {
                parser_advance(parser);
            }
            expr = call;
        }
        while (parser->current.type == TOKEN_OPERATOR && parser->current.lexeme[0] == '.') {
            ASTNode *bin = new_ast_node(AST_BINARY, parser->current);
            bin->left = expr;
            parser_advance(parser);
            bin->right = parse_primary(parser);
            expr = bin;
        }
        if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == ';') {
            parser_advance(parser);
        }
        return expr;
    } else {
        ASTNode *expr = id;
        while (parser->current.type == TOKEN_OPERATOR && parser->current.lexeme[0] == '.') {
            ASTNode *bin = new_ast_node(AST_BINARY, parser->current);
            bin->left = expr;
            parser_advance(parser);
            bin->right = parse_primary(parser);
            expr = bin;
        }
        if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == ';') {
            parser_advance(parser);
        }
        return expr;
    }
}

static int get_operator_precedence(const Token *tok) {
    if (tok->type != TOKEN_OPERATOR) return -1;
    if (tok->length == 1) {
        switch (tok->lexeme[0]) {
            case '*': case '/': return 10;
            case '+': case '-': return 9;
            case '>': case '<': return 8;
            case '=': case '!': return 7;
            case '.': return 11;
        }
    } else if (tok->length == 2) {
        if (strncmp(tok->lexeme, "==", 2) == 0 || strncmp(tok->lexeme, "!=", 2) == 0 || strncmp(tok->lexeme, ">=", 2) == 0 || strncmp(tok->lexeme, "<=", 2) == 0) return 7;
        if (strncmp(tok->lexeme, "&&", 2) == 0) return 6;
        if (strncmp(tok->lexeme, "||", 2) == 0) return 5;
    }
    return -1;
}

static ASTNode *parse_expression(Parser *parser, int min_prec) {
    ASTNode *left = parse_primary(parser);
    while (parser->current.type == TOKEN_OPERATOR) {
        int prec = get_operator_precedence(&parser->current);
        if (prec < min_prec) break;
        Token op_token = parser->current;
        parser_advance(parser);
        ASTNode *right = parse_expression(parser, prec + 1);
        ASTNode *op = new_ast_node(AST_BINARY, op_token);
        op->left = left;
        op->right = right;
        left = op;
    }
    return left;
}

static ASTNode *parse_primary(Parser *parser) {
    if (parser->current.type == TOKEN_NUMBER || parser->current.type == TOKEN_STRING || parser->current.type == TOKEN_CHAR) {
        ASTNode *node = new_ast_node(AST_LITERAL, parser->current);
        parser_advance(parser);
        return node;
    } else if (parser->current.type == TOKEN_IDENTIFIER) {
        return parse_call_or_identifier(parser);
    } else if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == '(') {
        parser_advance(parser);
        ASTNode *expr = parse_expression(parser, 0);
        if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == ')') {
            parser_advance(parser);
        }
        return expr;
    } else {
        parser_advance(parser);
        return NULL;
    }
}

static ASTNode *parse_call_or_identifier(Parser *parser) {
    ASTNode *expr = new_ast_node(AST_IDENTIFIER, parser->current);
    parser_advance(parser);
    while (1) {
        if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == '(') {
            ASTNode *call = new_ast_node(AST_CALL, parser->current);
            call->left = expr;
            parser_advance(parser);
            while (parser->current.type != TOKEN_PUNCTUATION || parser->current.lexeme[0] != ')') {
                if (parser->current.type == TOKEN_EOF) break;
                ASTNode *arg = parse_expression(parser, 0);
                call->args = (ASTNode **)realloc(call->args, sizeof(ASTNode *) * (call->arg_count + 1));
                call->args[call->arg_count++] = arg;
                if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == ',') {
                    parser_advance(parser);
                }
            }
            if (parser->current.type == TOKEN_PUNCTUATION && parser->current.lexeme[0] == ')') {
                parser_advance(parser);
            }
            expr = call;
        } else if (parser->current.type == TOKEN_OPERATOR && parser->current.lexeme[0] == '.') {
            ASTNode *bin = new_ast_node(AST_BINARY, parser->current);
            bin->left = expr;
            parser_advance(parser);
            bin->right = parse_primary(parser);
            expr = bin;
        } else {
            break;
        }
    }
    return expr;
}

static ASTNode *parse_unknown(Parser *parser) {
    parser_advance(parser);
    return NULL;
}

void free_ast(ASTNode *node) {
    if (!node) return;
    for (size_t i = 0; i < node->child_count; ++i) {
        free_ast(node->children[i]);
    }
    if (node->left) free_ast(node->left);
    if (node->right) free_ast(node->right);
    if (node->cond) free_ast(node->cond);
    if (node->body) free_ast(node->body);
    if (node->else_body) free_ast(node->else_body);
    for (size_t i = 0; i < node->arg_count; ++i) {
        free_ast(node->args[i]);
    }
    free(node->args);
    free(node->children);
    free(node);
} 