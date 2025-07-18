#ifndef KATLANGUAGE_CODEGEN_H
#define KATLANGUAGE_CODEGEN_H

#include "parser.h"
#include <stddef.h>

#define MAX_VARS 256

// Opcodes básicos inspirados en Lua
typedef enum {
    OP_LOADK,
    OP_LOADVAR,
    OP_STOREVAR,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_EQ,
    OP_LT,
    OP_LE,
    OP_JMP,
    OP_JMPIF,
    OP_CALL,
    OP_RETURN
} OpCode;

typedef struct {
    OpCode op;
    int a;
    int b;
    int c;
} Instruction;

typedef struct {
    char *name;
} VarInfo;

typedef struct {
    Instruction *code;
    size_t code_size;
    size_t code_capacity;
    int *constants;
    size_t const_size;
    size_t const_capacity;
    VarInfo vars[MAX_VARS];
    int var_count;
} Proto;

Proto *codegen_generate(const ASTNode *ast);
void codegen_free(Proto *proto);
void codegen_print(const Proto *proto);
int codegen_get_var_index(Proto *proto, const char *name, int create);

#endif 