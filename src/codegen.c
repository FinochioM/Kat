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
    gen->functions = NULL;
    gen->func_count = 0;
    gen->func_capacity = 0;
    gen->locals = NULL;
    gen->local_count = 0;
    gen->local_capacity = 0;
    gen->scope_level = 0;
    gen->next_reg = 0;
}

void codegen_free(CodeGen* gen) {
    free(gen->instructions);
    for (int i = 0; i < gen->const_count; i++) {
        free(gen->constants[i]);
    }
    free(gen->constants);
    
    for (int i = 0; i < gen->func_count; i++) {
        free(gen->functions[i].name);
        for (int j = 0; j < gen->functions[i].param_count; j++) {
            free(gen->functions[i].param_names[j]);
        }
        free(gen->functions[i].param_names);
    }
    free(gen->functions);
    
    for (int i = 0; i < gen->local_count; i++) {
        free(gen->locals[i].name);
    }
    free(gen->locals);
}

static void emit_instruction(CodeGen* gen, OpCode op, int A, int B, int C) {
    if (gen->count >= gen->capacity) {
        int new_capacity = gen->capacity < 8 ? 8 : gen->capacity * 2;
        gen->instructions = realloc(gen->instructions, new_capacity * sizeof(Instruction));
        gen->capacity = new_capacity;
    }
    
    gen->instructions[gen->count].opcode = op;
    gen->instructions[gen->count].A = A;
    gen->instructions[gen->count].B = B;
    gen->instructions[gen->count].C = C;
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

static int add_local(CodeGen* gen, const char* name) {
    if (gen->local_count >= gen->local_capacity) {
        int new_capacity = gen->local_capacity < 8 ? 8 : gen->local_capacity * 2;
        gen->locals = realloc(gen->locals, new_capacity * sizeof(LocalVar));
        gen->local_capacity = new_capacity;
    }
    
    gen->locals[gen->local_count].name = strdup(name);
    gen->locals[gen->local_count].reg = gen->next_reg++;
    gen->locals[gen->local_count].scope_level = gen->scope_level;
    
    return gen->local_count++;
}

static int find_local(CodeGen* gen, const char* name) {
    for (int i = gen->local_count - 1; i >= 0; i--) {
        if (strcmp(gen->locals[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void enter_scope(CodeGen* gen) {
    gen->scope_level++;
}

static void exit_scope(CodeGen* gen) {
    while (gen->local_count > 0 && 
           gen->locals[gen->local_count - 1].scope_level >= gen->scope_level) {
        free(gen->locals[gen->local_count - 1].name);
        gen->local_count--;
        gen->next_reg--;
    }
    gen->scope_level--;
}

static int generate_expression(CodeGen* gen, ASTNode* node) {
    switch (node->type) {
        case AST_LITERAL_EXPR: {
            int reg = gen->next_reg++;
            int const_idx = add_constant(gen, node->literal_expr.value);
            emit_instruction(gen, OP_LOADK, reg, const_idx, 0);
            return reg;
        }
        case AST_IDENTIFIER_EXPR: {
            int local_idx = find_local(gen, node->identifier_expr.name);
            if (local_idx != -1) {
                return gen->locals[local_idx].reg;
            } else {
                int reg = gen->next_reg++;
                int const_idx = add_constant(gen, node->identifier_expr.name);
                emit_instruction(gen, OP_LOADGLOBAL, reg, const_idx, 0);
                return reg;
            }
        }
        case AST_BINARY_EXPR: {
            int left_reg = generate_expression(gen, node->binary_expr.left);
            int right_reg = generate_expression(gen, node->binary_expr.right);
            int result_reg = gen->next_reg++;
            
            switch (node->binary_expr.operator) {
                case TOKEN_PLUS:
                    emit_instruction(gen, OP_ADD, result_reg, left_reg, right_reg);
                    break;
                case TOKEN_MINUS:
                    emit_instruction(gen, OP_SUB, result_reg, left_reg, right_reg);
                    break;
                case TOKEN_STAR:
                    emit_instruction(gen, OP_MUL, result_reg, left_reg, right_reg);
                    break;
                case TOKEN_SLASH:
                    emit_instruction(gen, OP_DIV, result_reg, left_reg, right_reg);
                    break;
                case TOKEN_GREATER:
                    emit_instruction(gen, OP_LT, result_reg, right_reg, left_reg);
                    break;
                case TOKEN_LESS:
                    emit_instruction(gen, OP_LT, result_reg, left_reg, right_reg);
                    break;
                case TOKEN_GREATER_EQUAL:
                    emit_instruction(gen, OP_LE, result_reg, right_reg, left_reg);
                    break;
                case TOKEN_LESS_EQUAL:
                    emit_instruction(gen, OP_LE, result_reg, left_reg, right_reg);
                    break;
                case TOKEN_DOUBLE_EQUAL:
                    emit_instruction(gen, OP_EQ, result_reg, left_reg, right_reg);
                    break;
                default:
                    break;
            }
            return result_reg;
        }
        case AST_CALL_EXPR: {
            int func_idx = -1;
            for (int i = 0; i < gen->func_count; i++) {
                if (strcmp(gen->functions[i].name, node->call_expr.name) == 0) {
                    func_idx = i;
                    break;
                }
            }
            
            int base_reg = gen->next_reg;
            
            for (int i = 0; i < node->call_expr.args->count; i++) {
                int arg_reg = generate_expression(gen, node->call_expr.args->nodes[i]);
                if (arg_reg != base_reg + i) {
                    emit_instruction(gen, OP_MOVE, base_reg + i, arg_reg, 0);
                }
            }
            
            gen->next_reg = base_reg + node->call_expr.args->count;
            
            int result_reg = gen->next_reg++;
            
            if (func_idx != -1) {
                emit_instruction(gen, OP_CALL_FUNC, result_reg, base_reg, func_idx);
            } else {
                int const_idx = add_constant(gen, node->call_expr.name);
                emit_instruction(gen, OP_CALL, result_reg, const_idx, base_reg);
            }
            
            return result_reg;
        }
        default:
            return 0;
    }
}

static void generate_statement(CodeGen* gen, ASTNode* node) {
    switch (node->type) {
        case AST_VAR_DECL: {
            if (node->var_decl.value) {
                int value_reg = generate_expression(gen, node->var_decl.value);
                int var_idx = add_local(gen, node->var_decl.name);
                int var_reg = gen->locals[var_idx].reg;
                
                if (value_reg != var_reg) {
                    emit_instruction(gen, OP_MOVE, var_reg, value_reg, 0);
                }
            }
            break;
        }
        case AST_ASSIGN_STMT: {
            int value_reg = generate_expression(gen, node->assign_stmt.value);
            int local_idx = find_local(gen, node->assign_stmt.name);
            
            if (local_idx != -1) {
                int var_reg = gen->locals[local_idx].reg;
                emit_instruction(gen, OP_MOVE, var_reg, value_reg, 0);
            } else {
                int const_idx = add_constant(gen, node->assign_stmt.name);
                emit_instruction(gen, OP_SETGLOBAL, value_reg, const_idx, 0);
            }
            break;
        }
        case AST_IF_STMT: {
            int cond_reg = generate_expression(gen, node->if_stmt.condition);
            int jump_addr = gen->count;
            emit_instruction(gen, OP_TEST, cond_reg, 0, 0);
            
            generate_statement(gen, node->if_stmt.then_stmt);
            
            gen->instructions[jump_addr].B = gen->count;
            break;
        }
        case AST_WHILE_STMT: {
            int loop_start = gen->count;
            
            int cond_reg = generate_expression(gen, node->while_stmt.condition);
            int jump_addr = gen->count;
            emit_instruction(gen, OP_TEST, cond_reg, 0, 0);
            
            generate_statement(gen, node->while_stmt.body);
            
            emit_instruction(gen, OP_JMP, loop_start - gen->count - 1, 0, 0);
            
            gen->instructions[jump_addr].B = gen->count;
            break;
        }
        case AST_RETURN_STMT: {
            int return_reg = 0;
            if (node->return_stmt.value) {
                return_reg = generate_expression(gen, node->return_stmt.value);
            }
            emit_instruction(gen, OP_RETURN, return_reg, 0, 0);
            break;
        }
        case AST_BLOCK_STMT: {
            enter_scope(gen);
            for (int i = 0; i < node->block_stmt.statements->count; i++) {
                generate_statement(gen, node->block_stmt.statements->nodes[i]);
            }
            exit_scope(gen);
            break;
        }
        case AST_EXPR_STMT: {
            generate_expression(gen, node->expr_stmt.expression);
            break;
        }
        default:
            break;
    }
}

static void generate_function(CodeGen* gen, ASTNode* proc_node) {
    int saved_local_count = gen->local_count;
    int saved_next_reg = gen->next_reg;
    int saved_scope_level = gen->scope_level;
    
    gen->scope_level = 0;
    gen->next_reg = 0;
    
    for (int i = 0; i < proc_node->proc_decl.params->count; i++) {
        ASTNode* param = proc_node->proc_decl.params->nodes[i];
        add_local(gen, param->param_decl.name);
    }
    
    generate_statement(gen, proc_node->proc_decl.body);
    
    gen->local_count = saved_local_count;
    gen->next_reg = saved_next_reg;
    gen->scope_level = saved_scope_level;
}

static int add_function(CodeGen* gen, const char* name, int start_addr, int param_count, ASTNode* proc_node) {
    if (gen->func_count >= gen->func_capacity) {
        int new_capacity = gen->func_capacity < 8 ? 8 : gen->func_capacity * 2;
        gen->functions = realloc(gen->functions, new_capacity * sizeof(Function));
        gen->func_capacity = new_capacity;
    }
    
    gen->functions[gen->func_count].name = strdup(name);
    gen->functions[gen->func_count].start_addr = start_addr;
    gen->functions[gen->func_count].param_count = param_count;
    gen->functions[gen->func_count].max_stack_size = 8;
    
    gen->functions[gen->func_count].param_names = malloc(param_count * sizeof(char*));
    for (int i = 0; i < param_count; i++) {
        ASTNode* param = proc_node->proc_decl.params->nodes[i];
        gen->functions[gen->func_count].param_names[i] = strdup(param->param_decl.name);
    }
    
    return gen->func_count++;
}

void generate_code(CodeGen* gen, ASTNode* node) {
    if (node->type == AST_PROGRAM) {
        int jump_addr = gen->count;
        emit_instruction(gen, OP_JMP, 0, 0, 0);
        
        for (int i = 0; i < node->program.declarations->count; i++) {
            ASTNode* decl = node->program.declarations->nodes[i];
            if (decl->type == AST_PROC_DECL) {
                int func_start = gen->count;
                add_function(gen, decl->proc_decl.name, func_start, decl->proc_decl.params->count, decl);
                generate_function(gen, decl);
            }
        }
        
        gen->instructions[jump_addr].A = gen->count - jump_addr - 1;
        
        int main_func_idx = -1;
        for (int i = 0; i < gen->func_count; i++) {
            if (strcmp(gen->functions[i].name, "main") == 0) {
                main_func_idx = i;
                break;
            }
        }
        
        if (main_func_idx != -1) {
            emit_instruction(gen, OP_CALL_FUNC, 0, 0, main_func_idx);
        }
    }
    
    emit_instruction(gen, OP_HALT, 0, 0, 0);
}

void print_bytecode(CodeGen* gen) {
    printf("=== BYTECODE ===\n");
    printf("Constants:\n");
    for (int i = 0; i < gen->const_count; i++) {
        printf("  [%d] %s\n", i, gen->constants[i]);
    }
    
    printf("\nFunctions:\n");
    for (int i = 0; i < gen->func_count; i++) {
        printf("  [%d] %s (params: %d, start: %d)\n", i, gen->functions[i].name, 
               gen->functions[i].param_count, gen->functions[i].start_addr);
    }
    
    printf("\nInstructions:\n");
    for (int i = 0; i < gen->count; i++) {
        printf("  %d: ", i);
        switch (gen->instructions[i].opcode) {
            case OP_MOVE: printf("MOVE R[%d] R[%d]\n", gen->instructions[i].A, gen->instructions[i].B); break;
            case OP_LOADK: printf("LOADK R[%d] K[%d]\n", gen->instructions[i].A, gen->instructions[i].B); break;
            case OP_LOADGLOBAL: printf("LOADGLOBAL R[%d] K[%d]\n", gen->instructions[i].A, gen->instructions[i].B); break;
            case OP_SETGLOBAL: printf("SETGLOBAL R[%d] K[%d]\n", gen->instructions[i].A, gen->instructions[i].B); break;
            case OP_CALL: printf("CALL R[%d] K[%d] R[%d]\n", gen->instructions[i].A, gen->instructions[i].B, gen->instructions[i].C); break;
            case OP_CALL_FUNC: printf("CALL_FUNC R[%d] R[%d] F[%d]\n", gen->instructions[i].A, gen->instructions[i].B, gen->instructions[i].C); break;
            case OP_RETURN: printf("RETURN R[%d]\n", gen->instructions[i].A); break;
            case OP_JMP: printf("JMP %d\n", gen->instructions[i].A); break;
            case OP_TEST: printf("TEST R[%d] %d\n", gen->instructions[i].A, gen->instructions[i].B); break;
            case OP_ADD: printf("ADD R[%d] R[%d] R[%d]\n", gen->instructions[i].A, gen->instructions[i].B, gen->instructions[i].C); break;
            case OP_SUB: printf("SUB R[%d] R[%d] R[%d]\n", gen->instructions[i].A, gen->instructions[i].B, gen->instructions[i].C); break;
            case OP_MUL: printf("MUL R[%d] R[%d] R[%d]\n", gen->instructions[i].A, gen->instructions[i].B, gen->instructions[i].C); break;
            case OP_DIV: printf("DIV R[%d] R[%d] R[%d]\n", gen->instructions[i].A, gen->instructions[i].B, gen->instructions[i].C); break;
            case OP_LT: printf("LT R[%d] R[%d] R[%d]\n", gen->instructions[i].A, gen->instructions[i].B, gen->instructions[i].C); break;
            case OP_LE: printf("LE R[%d] R[%d] R[%d]\n", gen->instructions[i].A, gen->instructions[i].B, gen->instructions[i].C); break;
            case OP_EQ: printf("EQ R[%d] R[%d] R[%d]\n", gen->instructions[i].A, gen->instructions[i].B, gen->instructions[i].C); break;
            case OP_HALT: printf("HALT\n"); break;
        }
    }
}