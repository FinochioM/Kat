#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GC_HEAP_GROW_FACTOR 2

void vm_init(VM* vm) {
    vm->code = NULL;
    vm->ip = 0;
    vm->reg_top = 0;
    vm->global_count = 0;
    vm->call_stack_top = 0;
    vm->objects = NULL;
    vm->bytes_allocated = 0;
    vm->next_gc = 1024 * 1024;
    
    for (int i = 0; i < GLOBALS_MAX; i++) {
        vm->global_names[i] = NULL;
    }
}

static GCObject* allocate_object(VM* vm, size_t size, GCType type) {
    GCObject* object = (GCObject*)malloc(size);
    object->type = type;
    object->marked = 0;
    object->next = vm->objects;
    vm->objects = object;
    
    vm->bytes_allocated += size;
    
    if (vm->bytes_allocated > vm->next_gc) {
        collect_garbage(vm);
    }
    
    return object;
}

static GCString* allocate_string(VM* vm, const char* chars, int length) {
    GCString* string = (GCString*)allocate_object(vm, sizeof(GCString), GC_STRING);
    string->length = length;
    string->chars = malloc(length + 1);
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    return string;
}

static void free_object(VM* vm, GCObject* object) {
    switch (object->type) {
        case GC_STRING: {
            GCString* string = (GCString*)object;
            free(string->chars);
            vm->bytes_allocated -= sizeof(GCString);
            break;
        }
        case GC_FUNCTION:
            vm->bytes_allocated -= sizeof(GCObject);
            break;
    }
    free(object);
}

void vm_free(VM* vm) {
    GCObject* object = vm->objects;
    while (object != NULL) {
        GCObject* next = object->next;
        free_object(vm, object);
        object = next;
    }
    
    for (int i = 0; i < vm->global_count; i++) {
        if (vm->global_names[i]) {
            free(vm->global_names[i]);
        }
    }
}

static void mark_object(GCObject* object) {
    if (object == NULL) return;
    if (object->marked) return;
    object->marked = 1;
}

static void mark_value(Value value) {
    if (value.type == VAL_STRING) {
        mark_object((GCObject*)value.as.string);
    }
}

static void mark_roots(VM* vm) {
    for (int i = 0; i < vm->reg_top; i++) {
        mark_value(vm->registers[i]);
    }
    
    for (int i = 0; i < vm->global_count; i++) {
        mark_value(vm->globals[i]);
    }
}

static void sweep(VM* vm) {
    GCObject** object = &vm->objects;
    while (*object != NULL) {
        if (!(*object)->marked) {
            GCObject* unreached = *object;
            *object = unreached->next;
            free_object(vm, unreached);
        } else {
            (*object)->marked = 0;
            object = &(*object)->next;
        }
    }
}

void collect_garbage(VM* vm) {
    int before = vm->bytes_allocated;
    mark_roots(vm);
    sweep(vm);
    vm->next_gc = vm->bytes_allocated * GC_HEAP_GROW_FACTOR;
}

static Value make_number(double value) {
    Value val;
    val.type = VAL_NUMBER;
    val.as.number = value;
    return val;
}

static Value make_string(VM* vm, const char* str) {
    Value val;
    val.type = VAL_STRING;
    val.as.string = allocate_string(vm, str, strlen(str));
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
        return make_nil();
    }
    
    return vm->globals[index];
}

static int is_truthy(Value value) {
    switch (value.type) {
        case VAL_NIL: return 0;
        case VAL_BOOL: return value.as.boolean;
        case VAL_NUMBER: return value.as.number != 0;
        case VAL_STRING: return value.as.string->length > 0;
        default: return 1;
    }
}

static void builtin_print(VM* vm, int arg_reg) {
    vm_print_value(vm->registers[arg_reg]);
    printf("\n");
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
            printf("%s", value.as.string->chars);
            break;
    }
}

int vm_run(VM* vm, CodeGen* code) {
    vm->code = code;
    vm->ip = 0;
    
    while (vm->ip < code->count) {
        Instruction instr = code->instructions[vm->ip];
        
        switch (instr.opcode) {
            case OP_MOVE: {
                vm->registers[instr.A] = vm->registers[instr.B];
                break;
            }
            
            case OP_LOADK: {
                const char* const_str = code->constants[instr.B];
                
                char* endptr;
                double num = strtod(const_str, &endptr);
                
                if (*endptr == '\0') {
                    vm->registers[instr.A] = make_number(num);
                } else if (const_str[0] == '"' && const_str[strlen(const_str) - 1] == '"') {
                    char* str = malloc(strlen(const_str) - 1);
                    strncpy(str, const_str + 1, strlen(const_str) - 2);
                    str[strlen(const_str) - 2] = '\0';
                    vm->registers[instr.A] = make_string(vm, str);
                    free(str);
                } else if (strcmp(const_str, "true") == 0) {
                    vm->registers[instr.A] = make_bool(1);
                } else if (strcmp(const_str, "false") == 0) {
                    vm->registers[instr.A] = make_bool(0);
                } else if (strcmp(const_str, "nil") == 0) {
                    vm->registers[instr.A] = make_nil();
                } else {
                    vm->registers[instr.A] = make_string(vm, const_str);
                }
                break;
            }
            
            case OP_LOADGLOBAL: {
                const char* name = code->constants[instr.B];
                vm->registers[instr.A] = get_global(vm, name);
                break;
            }
            
            case OP_SETGLOBAL: {
                const char* name = code->constants[instr.B];
                set_global(vm, name, vm->registers[instr.A]);
                break;
            }
            
            case OP_CALL: {
                const char* func_name = code->constants[instr.B];
                
                if (strcmp(func_name, "print") == 0) {
                    builtin_print(vm, instr.C);
                    vm->registers[instr.A] = make_nil();
                }
                break;
            }
            
            case OP_CALL_FUNC: {
                Function* func = &code->functions[instr.C];
                
                if (vm->call_stack_top >= CALL_STACK_MAX) {
                    fprintf(stderr, "Call stack overflow\n");
                    exit(1);
                }
                
                vm->call_stack[vm->call_stack_top].return_addr = vm->ip + 1;
                vm->call_stack[vm->call_stack_top].base_reg = instr.B;
                vm->call_stack[vm->call_stack_top].num_regs = func->max_stack_size;
                vm->call_stack_top++;
                
                for (int i = 0; i < func->param_count; i++) {
                    vm->registers[i] = vm->registers[instr.B + i];
                }
                
                vm->ip = func->start_addr - 1;
                break;
            }
            
            case OP_RETURN: {
                Value result = vm->registers[instr.A];
                
                if (vm->call_stack_top > 0) {
                    vm->call_stack_top--;
                    vm->ip = vm->call_stack[vm->call_stack_top].return_addr - 1;
                    
                    Instruction call_instr = code->instructions[vm->call_stack[vm->call_stack_top].return_addr - 1];
                    vm->registers[call_instr.A] = result;
                } else {
                    vm_print_value(result);
                    printf("\n");
                    return 0;
                }
                break;
            }
            
            case OP_JMP: {
                vm->ip += instr.A;
                break;
            }
            
            case OP_TEST: {
                if (!is_truthy(vm->registers[instr.A])) {
                    vm->ip = instr.B - 1;
                }
                break;
            }
            
            case OP_ADD: {
                Value left = vm->registers[instr.B];
                Value right = vm->registers[instr.C];
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    vm->registers[instr.A] = make_number(left.as.number + right.as.number);
                }
                break;
            }
            
            case OP_SUB: {
                Value left = vm->registers[instr.B];
                Value right = vm->registers[instr.C];
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    vm->registers[instr.A] = make_number(left.as.number - right.as.number);
                }
                break;
            }
            
            case OP_MUL: {
                Value left = vm->registers[instr.B];
                Value right = vm->registers[instr.C];
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    vm->registers[instr.A] = make_number(left.as.number * right.as.number);
                }
                break;
            }
            
            case OP_DIV: {
                Value left = vm->registers[instr.B];
                Value right = vm->registers[instr.C];
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    vm->registers[instr.A] = make_number(left.as.number / right.as.number);
                }
                break;
            }
            
            case OP_LT: {
                Value left = vm->registers[instr.B];
                Value right = vm->registers[instr.C];
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    vm->registers[instr.A] = make_bool(left.as.number < right.as.number);
                }
                break;
            }
            
            case OP_LE: {
                Value left = vm->registers[instr.B];
                Value right = vm->registers[instr.C];
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    vm->registers[instr.A] = make_bool(left.as.number <= right.as.number);
                }
                break;
            }
            
            case OP_EQ: {
                Value left = vm->registers[instr.B];
                Value right = vm->registers[instr.C];
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    vm->registers[instr.A] = make_bool(left.as.number == right.as.number);
                }
                break;
            }
            
            case OP_HALT: {
                return 0;
            }
            
            default:
                fprintf(stderr, "Unknown opcode %d\n", instr.opcode);
                return -1;
        }
        
        vm->ip++;
    }
    
    return 0;
}