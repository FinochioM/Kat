#include "semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *my_strndup(const char *s, size_t n) {
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

static SymbolTable *symboltable_new(SymbolTable *parent) {
    SymbolTable *table = (SymbolTable *)calloc(1, sizeof(SymbolTable));
    table->head = NULL;
    table->parent = parent;
    return table;
}

static void symboltable_free(SymbolTable *table) {
    Symbol *sym = table->head;
    while (sym) {
        Symbol *next = sym->next;
        free(sym->name);
        free(sym);
        sym = next;
    }
    free(table);
}

static Symbol *symboltable_lookup(SymbolTable *table, const char *name) {
    for (SymbolTable *t = table; t; t = t->parent) {
        for (Symbol *sym = t->head; sym; sym = sym->next) {
            if (strcmp(sym->name, name) == 0) return sym;
        }
    }
    return NULL;
}

static int symboltable_insert(SymbolTable *table, SymbolKind kind, const char *name) {
    for (Symbol *sym = table->head; sym; sym = sym->next) {
        if (strcmp(sym->name, name) == 0) return 0;
    }
    Symbol *sym = (Symbol *)calloc(1, sizeof(Symbol));
    sym->kind = kind;
    sym->name = strdup(name);
    sym->next = table->head;
    table->head = sym;
    return 1;
}

static void semantic_check_node(ASTNode *node, SemanticContext *ctx);

static void semantic_check_block(ASTNode *node, SemanticContext *ctx) {
    SymbolTable *prev = ctx->current_scope;
    ctx->current_scope = symboltable_new(prev);
    for (size_t i = 0; i < node->child_count; ++i) {
        semantic_check_node(node->children[i], ctx);
    }
    symboltable_free(ctx->current_scope);
    ctx->current_scope = prev;
}

static void semantic_check_var_decl(ASTNode *node, SemanticContext *ctx) {
    if (!node->left || node->left->type != AST_IDENTIFIER) return;
    char *name = my_strndup(node->left->token.lexeme, node->left->token.length);
    if (!symboltable_insert(ctx->current_scope, SYMBOL_VAR, name)) {
        printf("[Semantic Error] Variable '%s' already declared in this scope (line %d)\n", name, node->left->token.line);
        ctx->error_count++;
    }
    free(name);
    if (node->right) semantic_check_node(node->right, ctx);
}

static void semantic_check_assign(ASTNode *node, SemanticContext *ctx) {
    if (!node->left || node->left->type != AST_IDENTIFIER) return;
    char *name = my_strndup(node->left->token.lexeme, node->left->token.length);
    if (!symboltable_lookup(ctx->current_scope, name)) {
        printf("[Semantic Error] Variable '%s' not declared in this scope (line %d)\n", name, node->left->token.line);
        ctx->error_count++;
    }
    free(name);
    if (node->right) semantic_check_node(node->right, ctx);
}

static void semantic_check_identifier(ASTNode *node, SemanticContext *ctx) {
    char *name = my_strndup(node->token.lexeme, node->token.length);
    if (!symboltable_lookup(ctx->current_scope, name)) {
        printf("[Semantic Error] Variable '%s' not declared in this scope (line %d)\n", name, node->token.line);
        ctx->error_count++;
    }
    free(name);
}

static void semantic_check_proc(ASTNode *node, SemanticContext *ctx) {
    if (node->child_count > 0 && node->children[0]->type == AST_IDENTIFIER) {
        char *name = my_strndup(node->children[0]->token.lexeme, node->children[0]->token.length);
        if (!symboltable_insert(ctx->current_scope, SYMBOL_FUNC, name)) {
            printf("[Semantic Error] Function '%s' already declared in this scope (line %d)\n", name, node->children[0]->token.line);
            ctx->error_count++;
        }
        free(name);
    }
    for (size_t i = 1; i < node->child_count; ++i) {
        semantic_check_node(node->children[i], ctx);
    }
}

static void semantic_check_call(ASTNode *node, SemanticContext *ctx) {
    if (node->left) semantic_check_node(node->left, ctx);
    for (size_t i = 0; i < node->arg_count; ++i) {
        semantic_check_node(node->args[i], ctx);
    }
}

static void semantic_check_node(ASTNode *node, SemanticContext *ctx) {
    if (!node) return;
    switch (node->type) {
        case AST_BLOCK:
            semantic_check_block(node, ctx);
            break;
        case AST_VAR_DECL:
            semantic_check_var_decl(node, ctx);
            break;
        case AST_ASSIGN:
            semantic_check_assign(node, ctx);
            break;
        case AST_IDENTIFIER:
            semantic_check_identifier(node, ctx);
            break;
        case AST_PROC:
            semantic_check_proc(node, ctx);
            break;
        case AST_CALL:
            semantic_check_call(node, ctx);
            break;
        case AST_IF:
            if (node->cond) semantic_check_node(node->cond, ctx);
            if (node->body) semantic_check_node(node->body, ctx);
            if (node->else_body) semantic_check_node(node->else_body, ctx);
            break;
        case AST_FOR:
            if (node->cond) semantic_check_node(node->cond, ctx);
            if (node->body) semantic_check_node(node->body, ctx);
            break;
        case AST_RETURN:
            if (node->left) semantic_check_node(node->left, ctx);
            break;
        case AST_BINARY:
            if (node->left) semantic_check_node(node->left, ctx);
            if (node->right) semantic_check_node(node->right, ctx);
            break;
        case AST_LITERAL:
            break;
        default:
            for (size_t i = 0; i < node->child_count; ++i) {
                semantic_check_node(node->children[i], ctx);
            }
            break;
    }
}

void semantic_check(ASTNode *root) {
    SemanticContext ctx = {0};
    ctx.current_scope = symboltable_new(NULL);
    ctx.error_count = 0;
    semantic_check_node(root, &ctx);
    symboltable_free(ctx.current_scope);
    if (ctx.error_count == 0) {
        printf("\n[Semantic] No semantic errors found.\n");
    } else {
        printf("\n[Semantic] %d semantic error(s) found.\n", ctx.error_count);
    }
} 