#ifndef KATLANGUAGE_LEXER_H
#define KATLANGUAGE_LEXER_H

#include <stddef.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR,
    TOKEN_OPERATOR,
    TOKEN_KEYWORD,
    TOKEN_PUNCTUATION,
    TOKEN_COMMENT,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    const char *lexeme;
    size_t length;
    int line;
    int column;
} Token;

typedef struct {
    const char *source;
    size_t length;
    size_t position;
    int line;
    int column;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next_token(Lexer *lexer);

#endif 