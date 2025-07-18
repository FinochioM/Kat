#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static const char *keywords[] = {
    "package", "import", "proc", "struct", "enum", "map", "dynamic", "union", "bit_set", "distinct", "typeid", "defer", "using", "inline", "foreign", "break", "continue", "return", "if", "else", "for", "switch", "case", "in", "do", "or_else", "when", "where", "cast", "transmute", "nil", "true", "false", NULL
};

static int is_keyword(const char *lexeme, size_t length) {
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strlen(keywords[i]) == length && strncmp(lexeme, keywords[i], length) == 0) {
            return 1;
        }
    }
    return 0;
}

static void skip_whitespace(Lexer *lexer) {
    while (lexer->position < lexer->length) {
        char c = lexer->source[lexer->position];
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer->position++;
            lexer->column++;
        } else if (c == '\n') {
            lexer->position++;
            lexer->line++;
            lexer->column = 1;
        } else {
            break;
        }
    }
}

static int is_operator_char(char c) {
    return strchr("=+-*/<>!&|%^:~.?#$@", c) != NULL;
}

static int is_punctuation_char(char c) {
    return strchr("(){}[];,.", c) != NULL;
}

static Token make_token(Lexer *lexer, TokenType type, size_t start, size_t len, int line, int col) {
    Token token;
    token.type = type;
    token.lexeme = lexer->source + start;
    token.length = len;
    token.line = line;
    token.column = col;
    return token;
}

Token lexer_next_token(Lexer *lexer) {
    skip_whitespace(lexer);
    size_t start = lexer->position;
    int line = lexer->line;
    int col = lexer->column;

    if (lexer->position >= lexer->length) {
        return make_token(lexer, TOKEN_EOF, start, 0, line, col);
    }

    char c = lexer->source[lexer->position];

    if (c == '/') {
        if (lexer->position + 1 < lexer->length) {
            char next = lexer->source[lexer->position + 1];
            if (next == '/') {
                lexer->position += 2;
                lexer->column += 2;
                start = lexer->position;
                while (lexer->position < lexer->length && lexer->source[lexer->position] != '\n') {
                    lexer->position++;
                    lexer->column++;
                }
                return make_token(lexer, TOKEN_COMMENT, start, lexer->position - start, line, col);
            }
            if (next == '*') {
                lexer->position += 2;
                lexer->column += 2;
                start = lexer->position;
                while (lexer->position + 1 < lexer->length) {
                    if (lexer->source[lexer->position] == '*' && lexer->source[lexer->position + 1] == '/') {
                        size_t len = lexer->position - start;
                        lexer->position += 2;
                        lexer->column += 2;
                        return make_token(lexer, TOKEN_COMMENT, start, len, line, col);
                    }
                    if (lexer->source[lexer->position] == '\n') {
                        lexer->line++;
                        lexer->column = 1;
                    } else {
                        lexer->column++;
                    }
                    lexer->position++;
                }
                return make_token(lexer, TOKEN_UNKNOWN, start, lexer->position - start, line, col);
            }
        }
    }

    if (isalpha(c) || c == '_') {
        lexer->position++;
        lexer->column++;
        while (lexer->position < lexer->length) {
            char nc = lexer->source[lexer->position];
            if (isalnum(nc) || nc == '_') {
                lexer->position++;
                lexer->column++;
            } else {
                break;
            }
        }
        size_t len = lexer->position - start;
        if (is_keyword(lexer->source + start, len)) {
            return make_token(lexer, TOKEN_KEYWORD, start, len, line, col);
        } else {
            return make_token(lexer, TOKEN_IDENTIFIER, start, len, line, col);
        }
    }

    if (isdigit(c)) {
        int has_dot = 0;
        int has_exp = 0;
        lexer->position++;
        lexer->column++;
        while (lexer->position < lexer->length) {
            char nc = lexer->source[lexer->position];
            if (isdigit(nc)) {
                lexer->position++;
                lexer->column++;
            } else if (nc == '.' && !has_dot && !has_exp) {
                has_dot = 1;
                lexer->position++;
                lexer->column++;
            } else if ((nc == 'e' || nc == 'E') && !has_exp) {
                has_exp = 1;
                lexer->position++;
                lexer->column++;
                if (lexer->position < lexer->length && (lexer->source[lexer->position] == '+' || lexer->source[lexer->position] == '-')) {
                    lexer->position++;
                    lexer->column++;
                }
            } else {
                break;
            }
        }
        return make_token(lexer, TOKEN_NUMBER, start, lexer->position - start, line, col);
    }

    if (c == '"') {
        lexer->position++;
        lexer->column++;
        int terminated = 0;
        while (lexer->position < lexer->length) {
            char nc = lexer->source[lexer->position];
            if (nc == '\\' && lexer->position + 1 < lexer->length) {
                lexer->position += 2;
                lexer->column += 2;
            } else if (nc == '"') {
                lexer->position++;
                lexer->column++;
                terminated = 1;
                break;
            } else {
                if (nc == '\n') {
                    lexer->line++;
                    lexer->column = 1;
                } else {
                    lexer->column++;
                }
                lexer->position++;
            }
        }
        size_t len = lexer->position - start;
        if (!terminated) {
            return make_token(lexer, TOKEN_UNKNOWN, start, len, line, col);
        }
        return make_token(lexer, TOKEN_STRING, start, len, line, col);
    }

    if (c == '\'') {
        lexer->position++;
        lexer->column++;
        int terminated = 0;
        while (lexer->position < lexer->length) {
            char nc = lexer->source[lexer->position];
            if (nc == '\\' && lexer->position + 1 < lexer->length) {
                lexer->position += 2;
                lexer->column += 2;
            } else if (nc == '\'') {
                lexer->position++;
                lexer->column++;
                terminated = 1;
                break;
            } else {
                if (nc == '\n') {
                    lexer->line++;
                    lexer->column = 1;
                } else {
                    lexer->column++;
                }
                lexer->position++;
            }
        }
        size_t len = lexer->position - start;
        if (!terminated) {
            return make_token(lexer, TOKEN_UNKNOWN, start, len, line, col);
        }
        return make_token(lexer, TOKEN_CHAR, start, len, line, col);
    }

    if (is_operator_char(c)) {
        size_t op_len = 1;
        if (lexer->position + 1 < lexer->length) {
            char n1 = lexer->source[lexer->position + 1];
            char n2 = (lexer->position + 2 < lexer->length) ? lexer->source[lexer->position + 2] : 0;
            if ((c == ':' && n1 == ':') || (c == ':' && n1 == '=') || (c == '.' && n1 == '.') ||
                (c == '=' && n1 == '=') || (c == '!' && n1 == '=') || (c == '<' && n1 == '=') || (c == '>' && n1 == '=') ||
                (c == '-' && n1 == '>') || (c == '<' && n1 == '-') || (c == '&' && n1 == '&') || (c == '|' && n1 == '|')) {
                op_len = 2;
            } else if (c == '.' && n1 == '.' && n2 == '.') {
                op_len = 3;
            }
        }
        lexer->position += op_len;
        lexer->column += op_len;
        return make_token(lexer, TOKEN_OPERATOR, start, op_len, line, col);
    }

    if (is_punctuation_char(c)) {
        lexer->position++;
        lexer->column++;
        return make_token(lexer, TOKEN_PUNCTUATION, start, 1, line, col);
    }

    lexer->position++;
    lexer->column++;
    return make_token(lexer, TOKEN_UNKNOWN, start, 1, line, col);
}

void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->length = strlen(source);
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
} 