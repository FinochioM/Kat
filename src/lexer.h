#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define FIRST_RESERVED 256

typedef enum {
    TOKEN_SEMICOLON = ';',
    TOKEN_COMMA = ',',
    TOKEN_DOT = '.',
    TOKEN_LPAREN = '(',
    TOKEN_RPAREN = ')',
    TOKEN_LBRACE = '{',
    TOKEN_RBRACE = '}',
    TOKEN_LBRACKET = '[',
    TOKEN_RBRACKET = ']',
    TOKEN_PLUS = '+',
    TOKEN_MINUS = '-',
    TOKEN_STAR = '*',
    TOKEN_SLASH = '/',
    TOKEN_PERCENT = '%',
    TOKEN_CARET = '^',
    TOKEN_EQUAL = '=',
    TOKEN_LESS = '<',
    TOKEN_GREATER = '>',
    TOKEN_EXCLAMATION = '!',
    TOKEN_COLON = ':',
    TOKEN_QUESTION = '?',
    TOKEN_AMPERSAND = '&',
    TOKEN_PIPE = '|',
    TOKEN_TILDE = '~',
    
    TOKEN_IF = FIRST_RESERVED,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_DO,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_RETURN,
    TOKEN_PROC,
    TOKEN_STRUCT,
    TOKEN_ENUM,
    TOKEN_UNION,
    TOKEN_USING,
    TOKEN_IMPORT,
    TOKEN_PACKAGE,
    TOKEN_DEFER,
    TOKEN_CAST,
    TOKEN_TRANSMUTE,
    TOKEN_DISTINCT,
    TOKEN_OPAQUE,
    TOKEN_DYNAMIC,
    TOKEN_IN,
    TOKEN_NOT_IN,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NIL,
    TOKEN_AUTO_CAST,
    TOKEN_CONTEXT,
    TOKEN_FOREIGN,
    TOKEN_EXPORT,
    TOKEN_INLINE,
    TOKEN_NO_INLINE,
    TOKEN_WHEN,
    TOKEN_WHERE,
    TOKEN_CASE,
    TOKEN_SWITCH,
    TOKEN_FALLTHROUGH,
    TOKEN_VARIANT,
    TOKEN_SIZE_OF,
    TOKEN_ALIGN_OF,
    TOKEN_TYPE_OF,
    TOKEN_TYPEID_OF,
    TOKEN_OFFSET_OF,
    
    TOKEN_COLON_EQUAL,
    TOKEN_DOUBLE_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,
    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_OR,
    TOKEN_DOUBLE_COLON,
    TOKEN_ARROW,
    TOKEN_DOUBLE_DOT,
    TOKEN_TRIPLE_DOT,
    TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT,
    TOKEN_PLUS_EQUAL,
    TOKEN_MINUS_EQUAL,
    TOKEN_STAR_EQUAL,
    TOKEN_SLASH_EQUAL,
    TOKEN_PERCENT_EQUAL,
    TOKEN_CARET_EQUAL,
    TOKEN_AMPERSAND_EQUAL,
    TOKEN_PIPE_EQUAL,
    TOKEN_TILDE_EQUAL,
    TOKEN_SHIFT_LEFT_EQUAL,
    TOKEN_SHIFT_RIGHT_EQUAL,
    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,
    TOKEN_QUESTION_QUESTION,
    
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_CHARACTER,
    
    TOKEN_NEWLINE,
    TOKEN_EOF,
    TOKEN_INVALID
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
    int column;
} Token;

typedef struct {
    const char* start;
    const char* current;
    int line;
    int column;
    int start_column;
} Lexer;

void lexer_init(Lexer* lexer, const char* source);
Token lexer_next_token(Lexer* lexer);
const char* token_type_to_string(TokenType type);
void print_token(Token token);

#endif
