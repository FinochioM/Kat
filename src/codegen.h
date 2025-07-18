#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"

typedef enum {
    OP_MOVE,
    OP_LOADK,
    OP_LOADGLOBAL,
    OP_SETGLOBAL,
    OP_CALL,
    OP_CALL_FUNC,
    OP_RETURN,
    OP_JMP,
    OP_TEST,
    OP_EQ,
    OP_LT,
    OP_LE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_HALT
} OpCode;

typedef struct {
    OpCode opcode;
    int A, B, C;
} Instruction;

typedef struct {
    char* name;
    int start_addr;
    int param_count;
    int max_stack_size;
    char** param_names;
} Function;

typedef struct {
    char* name;
    int reg;
    int scope_level;
} LocalVar;

typedef struct {
    Instruction* instructions;
    int count;
    int capacity;
    char** constants;
    int const_count;
    int const_capacity;
    Function* functions;
    int func_count;
    int func_capacity;
    LocalVar* locals;
    int local_count;
    int local_capacity;
    int scope_level;
    int next_reg;
} CodeGen;

void codegen_init(CodeGen* gen);
void codegen_free(CodeGen* gen);
void generate_code(CodeGen* gen, ASTNode* node);
void print_bytecode(CodeGen* gen);

#endif