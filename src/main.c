#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "vm.h"

char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Could not allocate memory\n");
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(buffer, 1, size, file);
    buffer[bytes_read] = '\0';
    
    fclose(file);
    return buffer;
}

void test_lexer_only(const char* source) {
    printf("=== LEXER TEST ===\n");
    
    Lexer lexer;
    lexer_init(&lexer, source);
    
    Token token;
    do {
        token = lexer_next_token(&lexer);
        print_token(token);
    } while (token.type != TOKEN_EOF);
    
    printf("\n");
}

void test_parser(const char* source) {
    printf("=== PARSER TEST ===\n");
    
    Lexer lexer;
    lexer_init(&lexer, source);
    
    Parser parser;
    parser_init(&parser, &lexer);
    
    ASTNode* ast = parse_program(&parser);
    
    if (parser.had_error) {
        printf("Parser had errors.\n");
        return;
    }
    
    printf("AST:\n");
    print_ast(ast, 0);
    
    CodeGen gen;
    codegen_init(&gen);
    generate_code(&gen, ast);
    print_bytecode(&gen);
    
    printf("\n=== EXECUTION ===\n");
    VM vm;
    vm_init(&vm);
    vm_run(&vm, &gen);
    vm_free(&vm);
    
    codegen_free(&gen);
    free_ast(ast);
    printf("\n");
}


int main(int argc, char* argv[]) {
    printf("Kat Language Compiler Test\n");
    printf("==========================\n\n");
    
    const char* test_code = 
        "proc main() {\n"
        "    i := 0;\n"
        "    while i < 3 {\n"
        "        print(i);\n"
        "        i = i + 1;\n"
        "    }\n"
        "    return 0;\n"
        "}\n";
    
    if (argc > 1) {
        char* source = read_file(argv[1]);
        if (source) {
            test_parser(source);
            free(source);
        }
    } else {
        test_parser(test_code);
    }
    
    return 0;
}