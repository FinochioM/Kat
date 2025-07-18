#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void vm_init(VM* vm) {
    vm->code = NULL;
    vm->ip = 0;
    vm->stack_top = 0;
    vm->global_count = 0;
    
    for (int i = 0; i < GLOBALS_MAX; i++) {
        vm->global_names[i] = NULL;
    }
}

void vm_free(VM* vm) {
    for (int i = 0; i < vm->global_count; i++) {
        if (vm->global_names[i]) {
            free(vm->global_names[i]);
        }
        if (vm->globals[i].type == VAL_STRING) {
            free(vm->globals[i].as.string);
        }
    }
    
    // Free stack strings
    for (int i = 0; i < vm->stack_top; i++) {
        if (vm->stack[i].type == VAL_STRING) {
            free(vm->stack[i].as.string);
        }
    }
}

static void push(VM* vm, Value value) {
    if (vm->stack_top >= STACK_MAX) {
        fprintf(stderr, "Stack overflow\n");
        exit(1);
    }
    vm->stack[vm->stack_top++] = value;
}

static Value pop(VM* vm) {
    if (vm->stack_top <= 0) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    return vm->stack[--vm->stack_top];
}

static Value peek(VM* vm, int distance) {
    return vm->stack[vm->stack_top - 1 - distance];
}

static Value make_number(double value) {
    Value val;
    val.type = VAL_NUMBER;
    val.as.number = value;
    return val;
}

static Value make_string(const char* str) {
    Value val;
    val.type = VAL_STRING;
    val.as.string = strdup(str);
    return val;
}

static Value make_bool(int value) {
    Value val;
    val.type = VAL_BOOL;
    val.as.boolean = value;
    return val;
}

static Value make_nil(void) {
    Value val;
    val.type = VAL_NIL;
    return val;
}

static int find_global(VM* vm, const char* name) {
    for (int i = 0; i < vm->global_count; i++) {
        if (vm->global_names[i] && strcmp(vm->global_names[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

static void set_global(VM* vm, const char* name, Value value) {
    int index = find_global(vm, name);
    
    if (index == -1) {
        // Create new global
        if (vm->global_count >= GLOBALS_MAX) {
            fprintf(stderr, "Too many globals\n");
            exit(1);
        }
        
        index = vm->global_count++;
        vm->global_names[index] = strdup(name);
    }
    
    vm->globals[index] = value;
}

static Value get_global(VM* vm, const char* name) {
    int index = find_global(vm, name);
    
    if (index == -1) {
        fprintf(stderr, "Undefined variable '%s'\n", name);
        exit(1);
    }
    
    return vm->globals[index];
}

static int is_truthy(Value value) {
    switch (value.type) {
        case VAL_NIL: return 0;
        case VAL_BOOL: return value.as.boolean;
        case VAL_NUMBER: return value.as.number != 0;
        case VAL_STRING: return strlen(value.as.string) > 0;
        default: return 1;
    }
}

static void builtin_print(VM* vm) {
    Value arg = pop(vm);
    vm_print_value(arg);
    printf("\n");
    
    if (arg.type == VAL_STRING) {
        free(arg.as.string);
    }
}

static void call_builtin(VM* vm, const char* name) {
    if (strcmp(name, "print") == 0) {
        builtin_print(vm);
    } else {
        fprintf(stderr, "Unknown function '%s'\n", name);
        exit(1);
    }
}

void vm_print_value(Value value) {
    switch (value.type) {
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_BOOL:
            printf(value.as.boolean ? "true" : "false");
            break;
        case VAL_NUMBER:
            if (value.as.number == (int)value.as.number) {
                printf("%d", (int)value.as.number);
            } else {
                printf("%g", value.as.number);
            }
            break;
        case VAL_STRING:
            printf("%s", value.as.string);
            break;
    }
}

int vm_run(VM* vm, CodeGen* code) {
    vm->code = code;
    vm->ip = 0;
    
    while (vm->ip < code->count) {
        Instruction instr = code->instructions[vm->ip];
        
        switch (instr.opcode) {
            case OP_LOAD_CONST: {
                const char* const_str = code->constants[instr.operand];
                Value value;
                
                // Try to parse as number
                char* endptr;
                double num = strtod(const_str, &endptr);
                
                if (*endptr == '\0') {
                    // It's a number
                    value = make_number(num);
                } else if (const_str[0] == '"' && const_str[strlen(const_str) - 1] == '"') {
                    // It's a string (remove quotes)
                    char* str = malloc(strlen(const_str) - 1);
                    strncpy(str, const_str + 1, strlen(const_str) - 2);
                    str[strlen(const_str) - 2] = '\0';
                    value = make_string(str);
                    free(str);
                } else if (strcmp(const_str, "true") == 0) {
                    value = make_bool(1);
                } else if (strcmp(const_str, "false") == 0) {
                    value = make_bool(0);
                } else if (strcmp(const_str, "nil") == 0) {
                    value = make_nil();
                } else {
                    // Treat as string
                    value = make_string(const_str);
                }
                
                push(vm, value);
                break;
            }
            
            case OP_LOAD_VAR: {
                const char* name = code->constants[instr.operand];
                Value value = get_global(vm, name);
                push(vm, value);
                break;
            }
            
            case OP_STORE_VAR: {
                const char* name = code->constants[instr.operand];
                Value value = pop(vm);
                set_global(vm, name, value);
                break;
            }
            
            case OP_CALL: {
                const char* name = code->constants[instr.operand];
                call_builtin(vm, name);
                break;
            }
            
            case OP_COMPARE: {
                Value right = pop(vm);
                Value left = pop(vm);
                
                int result = 0;
                
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    switch (instr.operand) {
                        case TOKEN_GREATER:
                            result = left.as.number > right.as.number;
                            break;
                        case TOKEN_GREATER_EQUAL:
                            result = left.as.number >= right.as.number;
                            break;
                        case TOKEN_LESS:
                            result = left.as.number < right.as.number;
                            break;
                        case TOKEN_LESS_EQUAL:
                            result = left.as.number <= right.as.number;
                            break;
                        case TOKEN_DOUBLE_EQUAL:
                            result = left.as.number == right.as.number;
                            break;
                        case TOKEN_NOT_EQUAL:
                            result = left.as.number != right.as.number;
                            break;
                    }
                }
                
                push(vm, make_bool(result));
                break;
            }
            
            case OP_JUMP_IF_FALSE: {
                Value condition = pop(vm);
                if (!is_truthy(condition)) {
                    vm->ip = instr.operand - 1; // -1 because we increment at end
                }
                break;
            }
            
            case OP_JUMP: {
                vm->ip = instr.operand - 1; // -1 because we increment at end
                break;
            }
            
            case OP_RETURN: {
                if (vm->stack_top > 0) {
                    Value result = pop(vm);
                    vm_print_value(result);
                    printf("\n");
                }
                return 0;
            }
            
            case OP_HALT: {
                return 0;
            }
            
            case OP_ADD: {
                Value right = pop(vm);
                Value left = pop(vm);
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    push(vm, make_number(left.as.number + right.as.number));
                }
                break;
            }
            case OP_SUBTRACT: {
                Value right = pop(vm);
                Value left = pop(vm);
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    push(vm, make_number(left.as.number - right.as.number));
                }
                break;
            }
            case OP_MULTIPLY: {
                Value right = pop(vm);
                Value left = pop(vm);
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    push(vm, make_number(left.as.number * right.as.number));
                }
                break;
            }
            case OP_DIVIDE: {
                Value right = pop(vm);
                Value left = pop(vm);
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    push(vm, make_number(left.as.number / right.as.number));
                }
                break;
            }
            
            default:
                fprintf(stderr, "Unknown opcode %d\n", instr.opcode);
                return -1;
        }
        
        vm->ip++;
    }
    
    return 0;
}