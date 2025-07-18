#include "vm.h"
#include <stdio.h>
#include <string.h>

void vm_run(const Proto *proto) {
    int vars[MAX_VARS] = {0};
    int pc = 0;
    int running = 1;
    while (running && pc < (int)proto->code_size) {
        Instruction ins = proto->code[pc++];
        switch (ins.op) {
            case OP_LOADK:
                vars[ins.a] = proto->constants[ins.b];
                break;
            case OP_STOREVAR:
                break;
            case OP_LOADVAR:
                vars[ins.a] = vars[ins.b];
                break;
            case OP_ADD:
                vars[ins.a] = vars[ins.b] + vars[ins.c];
                break;
            case OP_SUB:
                vars[ins.a] = vars[ins.b] - vars[ins.c];
                break;
            case OP_MUL:
                vars[ins.a] = vars[ins.b] * vars[ins.c];
                break;
            case OP_DIV:
                vars[ins.a] = vars[ins.b] / vars[ins.c];
                break;
            case OP_JMP:
                pc += ins.b;
                break;
            case OP_JMPIF:
                if (!vars[ins.a]) pc += ins.b;
                break;
            case OP_RETURN:
                running = 0;
                break;
            default:
                break;
        }
    }
    printf("\nVM variables after execution:\n");
    for (int i = 0; i < proto->var_count; ++i) {
        printf("%s = %d\n", proto->vars[i].name, vars[i]);
    }
} 