#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void proto_emit(Proto *p, OpCode op, int a, int b, int c) {
    if (p->code_size == p->code_capacity) {
        p->code_capacity = p->code_capacity ? p->code_capacity * 2 : 16;
        p->code = (Instruction *)realloc(p->code, p->code_capacity * sizeof(Instruction));
    }
    p->code[p->code_size++] = (Instruction){op, a, b, c};
}

static int proto_add_const(Proto *p, int value) {
    if (p->const_size == p->const_capacity) {
        p->const_capacity = p->const_capacity ? p->const_capacity * 2 : 8;
        p->constants = (int *)realloc(p->constants, p->const_capacity * sizeof(int));
    }
    p->constants[p->const_size] = value;
    return (int)p->const_size++;
}

static void codegen_node(const ASTNode *node, Proto *p);

static void codegen_block(const ASTNode *node, Proto *p) {
    for (size_t i = 0; i < node->child_count; ++i) {
        codegen_node(node->children[i], p);
    }
}

static void codegen_var_decl(const ASTNode *node, Proto *p) {
    if (node->right && node->right->type == AST_LITERAL) {
        int kidx = proto_add_const(p, atoi(node->right->token.lexeme));
        proto_emit(p, OP_LOADK, 0, kidx, 0);
        proto_emit(p, OP_STOREVAR, 0, 0, 0);
    }
}

static void codegen_assign(const ASTNode *node, Proto *p) {
    if (node->right && node->right->type == AST_LITERAL) {
        int kidx = proto_add_const(p, atoi(node->right->token.lexeme));
        proto_emit(p, OP_LOADK, 0, kidx, 0);
        proto_emit(p, OP_STOREVAR, 0, 0, 0);
    }
}

static void codegen_if(const ASTNode *node, Proto *p) {
    codegen_node(node->cond, p);
    size_t jmpif_pos = p->code_size;
    proto_emit(p, OP_JMPIF, 0, 0, 0);
    codegen_node(node->body, p);
    if (node->else_body) {
        size_t jmp_pos = p->code_size;
        proto_emit(p, OP_JMP, 0, 0, 0);
        p->code[jmpif_pos].b = (int)(p->code_size - jmpif_pos);
        codegen_node(node->else_body, p);
        p->code[jmp_pos].b = (int)(p->code_size - jmp_pos);
    } else {
        p->code[jmpif_pos].b = (int)(p->code_size - jmpif_pos);
    }
}

static void codegen_node(const ASTNode *node, Proto *p) {
    if (!node) return;
    switch (node->type) {
        case AST_BLOCK:
            codegen_block(node, p);
            break;
        case AST_VAR_DECL:
            codegen_var_decl(node, p);
            break;
        case AST_ASSIGN:
            codegen_assign(node, p);
            break;
        case AST_IF:
            codegen_if(node, p);
            break;
        case AST_LITERAL:
            break;
        default:
            for (size_t i = 0; i < node->child_count; ++i) {
                codegen_node(node->children[i], p);
            }
            break;
    }
}

Proto *codegen_generate(const ASTNode *ast) {
    Proto *p = (Proto *)calloc(1, sizeof(Proto));
    codegen_node(ast, p);
    return p;
}

void codegen_free(Proto *proto) {
    if (!proto) return;
    free(proto->code);
    free(proto->constants);
    free(proto);
}

void codegen_print(const Proto *proto) {
    printf("\nBytecode:\n");
    for (size_t i = 0; i < proto->code_size; ++i) {
        Instruction *ins = &proto->code[i];
        printf("%3zu: OP_%d %d %d %d\n", i, ins->op, ins->a, ins->b, ins->c);
    }
    printf("Constants:\n");
    for (size_t i = 0; i < proto->const_size; ++i) {
        printf("  [%zu] = %d\n", i, proto->constants[i]);
    }
} 