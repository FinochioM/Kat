#ifndef VM_H
#define VM_H

#include "codegen.h"
#include <stdint.h>

#define STACK_MAX 256
#define GLOBALS_MAX 256

// Value types
typedef enum {
    VAL_NIL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_STRING
} ValueType;

// Value structure
typedef struct {
    ValueType type;
    union {
        int boolean;
        double number;
        char* string;
    } as;
} Value;

// VM structure
typedef struct {
    CodeGen* code;
    int ip; // instruction pointer
    Value stack[STACK_MAX];
    int stack_top;
    Value globals[GLOBALS_MAX];
    char* global_names[GLOBALS_MAX];
    int global_count;
} VM;

// VM functions
void vm_init(VM* vm);
void vm_free(VM* vm);
int vm_run(VM* vm, CodeGen* code);
void vm_print_value(Value value);

#endif // VM_H