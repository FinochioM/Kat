#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

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

void test_lexer_string(const char* source) {
    printf("Testing lexer with: \"%s\"\n", source);
    printf("==========================================\n");
    
    Lexer lexer;
    lexer_init(&lexer, source);
    
    Token token;
    do {
        token = lexer_next_token(&lexer);
        print_token(token);
    } while (token.type != TOKEN_EOF);
    
    printf("\n");
}

int main(int argc, char* argv[]) {
    printf("Kat Language Lexer Test\n");
    printf("=======================\n\n");
    
    if (argc > 1) {
        char* source = read_file(argv[1]);
        if (source) {
            test_lexer_string(source);
            free(source);
        }
    } else {
        const char* test_code = 
            "proc main() {\n"
            "    x := 42;\n"
            "    name := \"Hello, World!\";\n"
            "    if x > 0 {\n"
            "        print(name);\n"
            "    }\n"
            "    return 0;\n"
            "}\n";
        
        test_lexer_string(test_code);
    }
    
    return 0;
}