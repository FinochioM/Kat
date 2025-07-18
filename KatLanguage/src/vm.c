#include "vm.h"
#include <stdio.h>

void vm_run(const Proto *proto) {
    VM vm = {0};
    vm.pc = 0;
    int running = 1;
    while (running && vm.pc < (int)proto->code_size) {
        Instruction ins = proto->code[vm.pc++];
        switch (ins.op) {
            case OP_LOADK:
                vm.registers[ins.a] = proto->constants[ins.b];
                break;
            case OP_STOREVAR:
                break;
            case OP_ADD:
                vm.registers[ins.a] = vm.registers[ins.b] + vm.registers[ins.c];
                break;
            case OP_SUB:
                vm.registers[ins.a] = vm.registers[ins.b] - vm.registers[ins.c];
                break;
            case OP_MUL:
                vm.registers[ins.a] = vm.registers[ins.b] * vm.registers[ins.c];
                break;
            case OP_DIV:
                vm.registers[ins.a] = vm.registers[ins.b] / vm.registers[ins.c];
                break;
            case OP_JMP:
                vm.pc += ins.b;
                break;
            case OP_JMPIF:
                if (!vm.registers[ins.a]) vm.pc += ins.b;
                break;
            case OP_RETURN:
                running = 0;
                break;
            default:
                break;
        }
    }
    printf("\nVM registers after execution:\n");
    for (int i = 0; i < 8; ++i) {
        printf("R%d = %d\n", i, vm.registers[i]);
    }
} 