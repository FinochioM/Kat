#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"

// Bytecode opcodes
typedef enum {
    OP_LOAD_CONST,
    OP_LOAD_VAR,
    OP_STORE_VAR,
    OP_CALL,
    OP_RETURN,
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_COMPARE,
    OP_ADD,        // +
    OP_SUBTRACT,   // -
    OP_MULTIPLY,   // *
    OP_DIVIDE,     // /
    OP_HALT
} OpCode;

// Instruction structure
typedef struct {
    OpCode opcode;
    int operand;
} Instruction;

// Code generator
typedef struct {
    Instruction* instructions;
    int count;
    int capacity;
    char** constants;
    int const_count;
    int const_capacity;
} CodeGen;

void codegen_init(CodeGen* gen);
void codegen_free(CodeGen* gen);
void generate_code(CodeGen* gen, ASTNode* node);
void print_bytecode(CodeGen* gen);

#endif // CODEGEN_H