#ifndef KATLANGUAGE_VM_H
#define KATLANGUAGE_VM_H

#include "codegen.h"

typedef struct {
    int registers[16];
    int pc;
} VM;

void vm_run(const Proto *proto);

#endif 