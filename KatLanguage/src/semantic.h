#ifndef KATLANGUAGE_SEMANTIC_H
#define KATLANGUAGE_SEMANTIC_H

#include "parser.h"
#include <stddef.h>

typedef enum {
    SYMBOL_VAR,
    SYMBOL_FUNC,
    SYMBOL_TYPE
} SymbolKind;

typedef struct Symbol {
    SymbolKind kind;
    char *name;
    struct Symbol *next;
} Symbol;

typedef struct SymbolTable {
    Symbol *head;
    struct SymbolTable *parent;
} SymbolTable;

typedef struct {
    SymbolTable *current_scope;
    int error_count;
} SemanticContext;

void semantic_check(ASTNode *root);

#endif 