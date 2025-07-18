#include "codegen.h"
#include <stdlib.h>
#include <string.h>

void codegen_init(CodeGen* gen) {
    gen->instructions = NULL;
    gen->count = 0;
    gen->capacity = 0;
    gen->constants = NULL;
    gen->const_count = 0;
    gen->const_capacity = 0;
}

void codegen_free(CodeGen* gen) {
    free(gen->instructions);
    for (int i = 0; i < gen->const_count; i++) {
        free(gen->constants[i]);
    }
    free(gen->constants);
}

static void emit_instruction(CodeGen* gen, OpCode op, int operand) {
    if (gen->count >= gen->capacity) {
        int new_capacity = gen->capacity < 8 ? 8 : gen->capacity * 2;
        gen->instructions = realloc(gen->instructions, new_capacity * sizeof(Instruction));
        gen->capacity = new_capacity;
    }
    
    gen->instructions[gen->count].opcode = op;
    gen->instructions[gen->count].operand = operand;
    gen->count++;
}

static int add_constant(CodeGen* gen, const char* value) {
    if (gen->const_count >= gen->const_capacity) {
        int new_capacity = gen->const_capacity < 8 ? 8 : gen->const_capacity * 2;
        gen->constants = realloc(gen->constants, new_capacity * sizeof(char*));
        gen->const_capacity = new_capacity;
    }
    
    gen->constants[gen->const_count] = strdup(value);
    return gen->const_count++;
}

static void generate_expression(CodeGen* gen, ASTNode* node) {
    switch (node->type) {
        case AST_LITERAL_EXPR: {
            int const_idx = add_constant(gen, node->literal_expr.value);
            emit_instruction(gen, OP_LOAD_CONST, const_idx);
            break;
        }
        case AST_IDENTIFIER_EXPR: {
            int const_idx = add_constant(gen, node->identifier_expr.name);
            emit_instruction(gen, OP_LOAD_VAR, const_idx);
            break;
        }
        case AST_BINARY_EXPR: {
            generate_expression(gen, node->binary_expr.left);
            generate_expression(gen, node->binary_expr.right);
            
            switch (node->binary_expr.operator) {
                case TOKEN_PLUS: emit_instruction(gen, OP_ADD, 0); break;
                case TOKEN_MINUS: emit_instruction(gen, OP_SUBTRACT, 0); break;
                case TOKEN_STAR: emit_instruction(gen, OP_MULTIPLY, 0); break;
                case TOKEN_SLASH: emit_instruction(gen, OP_DIVIDE, 0); break;
                default: emit_instruction(gen, OP_COMPARE, node->binary_expr.operator); break;
            }
            break;
        }
        case AST_CALL_EXPR: {
            for (int i = 0; i < node->call_expr.args->count; i++) {
                generate_expression(gen, node->call_expr.args->nodes[i]);
            }
            int const_idx = add_constant(gen, node->call_expr.name);
            emit_instruction(gen, OP_CALL, const_idx);
            break;
        }
        default:
            break;
    }
}

static void generate_statement(CodeGen* gen, ASTNode* node) {
    switch (node->type) {
        case AST_VAR_DECL: {
            if (node->var_decl.value) {
                generate_expression(gen, node->var_decl.value);
                int const_idx = add_constant(gen, node->var_decl.name);
                emit_instruction(gen, OP_STORE_VAR, const_idx);
            }
            break;
        }
        case AST_EXPR_STMT: {
            generate_expression(gen, node->expr_stmt.expression);
            break;
        }
        case AST_IF_STMT: {
            generate_expression(gen, node->if_stmt.condition);
            int jump_addr = gen->count;
            emit_instruction(gen, OP_JUMP_IF_FALSE, 0); // placeholder
            
            generate_statement(gen, node->if_stmt.then_stmt);
            
            // Update jump address
            gen->instructions[jump_addr].operand = gen->count;
            break;
        }
        case AST_RETURN_STMT: {
            if (node->return_stmt.value) {
                generate_expression(gen, node->return_stmt.value);
            }
            emit_instruction(gen, OP_RETURN, 0);
            break;
        }
        case AST_BLOCK_STMT: {
            for (int i = 0; i < node->block_stmt.statements->count; i++) {
                generate_statement(gen, node->block_stmt.statements->nodes[i]);
            }
            break;
        }
        default:
            break;
    }
}

void generate_code(CodeGen* gen, ASTNode* node) {
    if (node->type == AST_PROGRAM) {
        for (int i = 0; i < node->program.declarations->count; i++) {
            ASTNode* decl = node->program.declarations->nodes[i];
            if (decl->type == AST_PROC_DECL) {
                generate_statement(gen, decl->proc_decl.body);
            }
        }
    }
    
    emit_instruction(gen, OP_HALT, 0);
}

void print_bytecode(CodeGen* gen) {
    printf("=== BYTECODE ===\n");
    printf("Constants:\n");
    for (int i = 0; i < gen->const_count; i++) {
        printf("  [%d] %s\n", i, gen->constants[i]);
    }
    
    printf("\nInstructions:\n");
    for (int i = 0; i < gen->count; i++) {
        printf("  %d: ", i);
        switch (gen->instructions[i].opcode) {
            case OP_LOAD_CONST: printf("LOAD_CONST %d\n", gen->instructions[i].operand); break;
            case OP_LOAD_VAR: printf("LOAD_VAR %d\n", gen->instructions[i].operand); break;
            case OP_STORE_VAR: printf("STORE_VAR %d\n", gen->instructions[i].operand); break;
            case OP_CALL: printf("CALL %d\n", gen->instructions[i].operand); break;
            case OP_RETURN: printf("RETURN\n"); break;
            case OP_JUMP_IF_FALSE: printf("JUMP_IF_FALSE %d\n", gen->instructions[i].operand); break;
            case OP_JUMP: printf("JUMP %d\n", gen->instructions[i].operand); break;
            case OP_COMPARE: printf("COMPARE %d\n", gen->instructions[i].operand); break;
            case OP_ADD: printf("ADD\n"); break;
            case OP_SUBTRACT: printf("SUBTRACT\n"); break;
            case OP_MULTIPLY: printf("MULTIPLY\n"); break;
            case OP_DIVIDE: printf("DIVIDE\n"); break;
            case OP_HALT: printf("HALT\n"); break;
        }
    }
}