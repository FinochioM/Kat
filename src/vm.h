#ifndef VM_H
#define VM_H

#include "codegen.h"
#include <stdint.h>

#define STACK_MAX 256
#define GLOBALS_MAX 256
#define CALL_STACK_MAX 256

typedef struct {
    int return_addr;
    int base_reg;     // registro base para esta función
    int num_regs;     // número de registros usados
} CallFrame;

typedef enum {
    VAL_NIL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_STRING
} ValueType;

typedef struct {
    ValueType type;
    union {
        int boolean;
        double number;
        char* string;
    } as;
} Value;

typedef struct {
    CodeGen* code;
    int ip;
    Value registers[STACK_MAX];  // registros en lugar de stack
    int reg_top;
    Value globals[GLOBALS_MAX];
    char* global_names[GLOBALS_MAX];
    int global_count;
    CallFrame call_stack[CALL_STACK_MAX];
    int call_stack_top;
} VM;

void vm_init(VM* vm);
void vm_free(VM* vm);
int vm_run(VM* vm, CodeGen* code);
void vm_print_value(Value value);

#endif // VM_H
