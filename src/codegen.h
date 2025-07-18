#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"

// Bytecode opcodes (similar a LUA)
typedef enum {
    OP_MOVE,         // R[A] := R[B]
    OP_LOADK,        // R[A] := K[Bx] 
    OP_LOADGLOBAL,   // R[A] := globals[K[Bx]]
    OP_SETGLOBAL,    // globals[K[Bx]] := R[A]
    OP_CALL,         // R[A](R[A+1], ..., R[A+B])
    OP_CALL_FUNC,    // Llamar función definida por usuario
    OP_RETURN,       // return R[A]
    OP_JMP,          // pc += sBx
    OP_TEST,         // if not R[A] then pc++
    OP_EQ,           // R[A] := R[B] == R[C]
    OP_LT,           // R[A] := R[B] < R[C]
    OP_LE,           // R[A] := R[B] <= R[C]
    OP_ADD,          // R[A] := R[B] + R[C]
    OP_SUB,          // R[A] := R[B] - R[C]
    OP_MUL,          // R[A] := R[B] * R[C]
    OP_DIV,          // R[A] := R[B] / R[C]
    OP_HALT
} OpCode;

// Instrucción con formato ABC similar a LUA
typedef struct {
    OpCode opcode;
    int A, B, C;
} Instruction;

// Función con tabla de parámetros
typedef struct {
    char* name;
    int start_addr;
    int param_count;
    int max_stack_size;
    char** param_names;  // nombres de parámetros
} Function;

// Compilador con scope de variables
typedef struct {
    char* name;
    int reg;  // registro asignado
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
    
    // Estado de compilación actual
    LocalVar* locals;
    int local_count;
    int local_capacity;
    int scope_level;
    int next_reg;  // próximo registro libre
} CodeGen;

void codegen_init(CodeGen* gen);
void codegen_free(CodeGen* gen);
void generate_code(CodeGen* gen, ASTNode* node);
void print_bytecode(CodeGen* gen);

#endif // CODEGEN_H