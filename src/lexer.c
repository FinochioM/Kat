#include "lexer.h"

static const char* keywords[] = {
    "if", "else", "for", "while", "do", "break", "continue", "return",
    "proc", "struct", "enum", "union", "using", "import", "package",
    "defer", "cast", "transmute", "distinct", "opaque", "dynamic",
    "in", "not_in", "true", "false", "nil", "auto_cast", "context",
    "foreign", "export", "inline", "no_inline", "when", "where",
    "case", "switch", "fallthrough", "variant", "size_of", "align_of",
    "type_of", "typeid_of", "offset_of"
};

static TokenType keyword_tokens[] = {
    TOKEN_IF, TOKEN_ELSE, TOKEN_FOR, TOKEN_WHILE, TOKEN_DO, TOKEN_BREAK,
    TOKEN_CONTINUE, TOKEN_RETURN, TOKEN_PROC, TOKEN_STRUCT, TOKEN_ENUM,
    TOKEN_UNION, TOKEN_USING, TOKEN_IMPORT, TOKEN_PACKAGE, TOKEN_DEFER,
    TOKEN_CAST, TOKEN_TRANSMUTE, TOKEN_DISTINCT, TOKEN_OPAQUE, TOKEN_DYNAMIC,
    TOKEN_IN, TOKEN_NOT_IN, TOKEN_TRUE, TOKEN_FALSE, TOKEN_NIL, TOKEN_AUTO_CAST,
    TOKEN_CONTEXT, TOKEN_FOREIGN, TOKEN_EXPORT, TOKEN_INLINE, TOKEN_NO_INLINE,
    TOKEN_WHEN, TOKEN_WHERE, TOKEN_CASE, TOKEN_SWITCH, TOKEN_FALLTHROUGH,
    TOKEN_VARIANT, TOKEN_SIZE_OF, TOKEN_ALIGN_OF, TOKEN_TYPE_OF, TOKEN_TYPEID_OF,
    TOKEN_OFFSET_OF
};

static int keyword_count = sizeof(keywords) / sizeof(keywords[0]);

void lexer_init(Lexer* lexer, const char* source) {
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
    lexer->start_column = 1;
}

static int is_at_end(Lexer* lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    
    if (*lexer->current == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    
    return *lexer->current++;
}

static char peek(Lexer* lexer) {
    return *lexer->current;
}

static char peek_next(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static int match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return 0;
    if (*lexer->current != expected) return 0;
    
    advance(lexer);
    return 1;
}

static void skip_whitespace(Lexer* lexer) {
    while (1) {
        char c = peek(lexer);
        
        if (c == ' ' || c == '\r' || c == '\t') {
            advance(lexer);
        } else if (c == '/' && peek_next(lexer) == '/') {
            while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                advance(lexer);
            }
        } else if (c == '/' && peek_next(lexer) == '*') {
            advance(lexer);
            advance(lexer);
            
            while (!is_at_end(lexer)) {
                if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                    advance(lexer);
                    advance(lexer);
                    break;
                }
                advance(lexer);
            }
        } else {
            break;
        }
    }
}

static Token make_token(Lexer* lexer, TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = (int)(lexer->current - lexer->start);
    token.line = lexer->line;
    token.column = lexer->start_column;
    
    return token;
}

static Token string_token(Lexer* lexer) {
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
        advance(lexer);
    }
    
    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_INVALID);
    }
    
    advance(lexer);
    return make_token(lexer, TOKEN_STRING);
}

static Token number_token(Lexer* lexer) {
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }
    
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        advance(lexer);
        
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }
    
    return make_token(lexer, TOKEN_NUMBER);
}

static Token identifier_token(Lexer* lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }
    
    int length = (int)(lexer->current - lexer->start);
    for (int i = 0; i < keyword_count; i++) {
        if (strlen(keywords[i]) == length && 
            strncmp(lexer->start, keywords[i], length) == 0) {
            return make_token(lexer, keyword_tokens[i]);
        }
    }
    
    return make_token(lexer, TOKEN_IDENTIFIER);
}

Token lexer_next_token(Lexer* lexer) {
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;
    lexer->start_column = lexer->column;
    
    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }
    
    char c = advance(lexer);
    
    if (isalpha(c) || c == '_') {
        return identifier_token(lexer);
    }
    
    if (isdigit(c)) {
        return number_token(lexer);
    }
    
    switch (c) {
        case '\n': return make_token(lexer, TOKEN_NEWLINE);
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);
        case '{': return make_token(lexer, TOKEN_LBRACE);
        case '}': return make_token(lexer, TOKEN_RBRACE);
        case '[': return make_token(lexer, TOKEN_LBRACKET);
        case ']': return make_token(lexer, TOKEN_RBRACKET);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '.': 
            if (match(lexer, '.')) {
                if (match(lexer, '.')) {
                    return make_token(lexer, TOKEN_TRIPLE_DOT);
                }
                return make_token(lexer, TOKEN_DOUBLE_DOT);
            }
            return make_token(lexer, TOKEN_DOT);
        case '-': 
            if (match(lexer, '>')) {
                return make_token(lexer, TOKEN_ARROW);
            }
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_MINUS_EQUAL);
            }
            if (match(lexer, '-')) {
                return make_token(lexer, TOKEN_MINUS_MINUS);
            }
            return make_token(lexer, TOKEN_MINUS);
        case '+': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_PLUS_EQUAL);
            }
            if (match(lexer, '+')) {
                return make_token(lexer, TOKEN_PLUS_PLUS);
            }
            return make_token(lexer, TOKEN_PLUS);
        case '/': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_SLASH_EQUAL);
            }
            return make_token(lexer, TOKEN_SLASH);
        case '*': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_STAR_EQUAL);
            }
            return make_token(lexer, TOKEN_STAR);
        case '%': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_PERCENT_EQUAL);
            }
            return make_token(lexer, TOKEN_PERCENT);
        case '!': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_NOT_EQUAL);
            }
            return make_token(lexer, TOKEN_EXCLAMATION);
        case '=': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_DOUBLE_EQUAL);
            }
            return make_token(lexer, TOKEN_EQUAL);
        case '<': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_LESS_EQUAL);
            }
            if (match(lexer, '<')) {
                if (match(lexer, '=')) {
                    return make_token(lexer, TOKEN_SHIFT_LEFT_EQUAL);
                }
                return make_token(lexer, TOKEN_SHIFT_LEFT);
            }
            return make_token(lexer, TOKEN_LESS);
        case '>': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_GREATER_EQUAL);
            }
            if (match(lexer, '>')) {
                if (match(lexer, '=')) {
                    return make_token(lexer, TOKEN_SHIFT_RIGHT_EQUAL);
                }
                return make_token(lexer, TOKEN_SHIFT_RIGHT);
            }
            return make_token(lexer, TOKEN_GREATER);
        case '&': 
            if (match(lexer, '&')) {
                return make_token(lexer, TOKEN_LOGICAL_AND);
            }
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_AMPERSAND_EQUAL);
            }
            return make_token(lexer, TOKEN_AMPERSAND);
        case '|': 
            if (match(lexer, '|')) {
                return make_token(lexer, TOKEN_LOGICAL_OR);
            }
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_PIPE_EQUAL);
            }
            return make_token(lexer, TOKEN_PIPE);
        case '^': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_CARET_EQUAL);
            }
            return make_token(lexer, TOKEN_CARET);
        case '~': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_TILDE_EQUAL);
            }
            return make_token(lexer, TOKEN_TILDE);
        case ':': 
            if (match(lexer, '=')) {
                return make_token(lexer, TOKEN_COLON_EQUAL);
            }
            if (match(lexer, ':')) {
                return make_token(lexer, TOKEN_DOUBLE_COLON);
            }
            return make_token(lexer, TOKEN_COLON);
        case '?': 
            if (match(lexer, '?')) {
                return make_token(lexer, TOKEN_QUESTION_QUESTION);
            }
            return make_token(lexer, TOKEN_QUESTION);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case '"': return string_token(lexer);
        case '\'': {
            if (is_at_end(lexer)) return make_token(lexer, TOKEN_INVALID);
            advance(lexer);
            if (peek(lexer) != '\'') return make_token(lexer, TOKEN_INVALID);
            advance(lexer);
            return make_token(lexer, TOKEN_CHARACTER);
        }
    }
    
    return make_token(lexer, TOKEN_INVALID);
}

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_IF: return "if";
        case TOKEN_ELSE: return "else";
        case TOKEN_FOR: return "for";
        case TOKEN_WHILE: return "while";
        case TOKEN_DO: return "do";
        case TOKEN_BREAK: return "break";
        case TOKEN_CONTINUE: return "continue";
        case TOKEN_RETURN: return "return";
        case TOKEN_PROC: return "proc";
        case TOKEN_STRUCT: return "struct";
        case TOKEN_ENUM: return "enum";
        case TOKEN_UNION: return "union";
        case TOKEN_USING: return "using";
        case TOKEN_IMPORT: return "import";
        case TOKEN_PACKAGE: return "package";
        case TOKEN_DEFER: return "defer";
        case TOKEN_CAST: return "cast";
        case TOKEN_TRANSMUTE: return "transmute";
        case TOKEN_DISTINCT: return "distinct";
        case TOKEN_OPAQUE: return "opaque";
        case TOKEN_DYNAMIC: return "dynamic";
        case TOKEN_IN: return "in";
        case TOKEN_NOT_IN: return "not_in";
        case TOKEN_TRUE: return "true";
        case TOKEN_FALSE: return "false";
        case TOKEN_NIL: return "nil";
        case TOKEN_AUTO_CAST: return "auto_cast";
        case TOKEN_CONTEXT: return "context";
        case TOKEN_FOREIGN: return "foreign";
        case TOKEN_EXPORT: return "export";
        case TOKEN_INLINE: return "inline";
        case TOKEN_NO_INLINE: return "no_inline";
        case TOKEN_WHEN: return "when";
        case TOKEN_WHERE: return "where";
        case TOKEN_CASE: return "case";
        case TOKEN_SWITCH: return "switch";
        case TOKEN_FALLTHROUGH: return "fallthrough";
        case TOKEN_VARIANT: return "variant";
        case TOKEN_SIZE_OF: return "size_of";
        case TOKEN_ALIGN_OF: return "align_of";
        case TOKEN_TYPE_OF: return "type_of";
        case TOKEN_TYPEID_OF: return "typeid_of";
        case TOKEN_OFFSET_OF: return "offset_of";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_CHARACTER: return "CHARACTER";
        case TOKEN_COLON_EQUAL: return ":=";
        case TOKEN_DOUBLE_EQUAL: return "==";
        case TOKEN_NOT_EQUAL: return "!=";
        case TOKEN_LESS_EQUAL: return "<=";
        case TOKEN_GREATER_EQUAL: return ">=";
        case TOKEN_LOGICAL_AND: return "&&";
        case TOKEN_LOGICAL_OR: return "||";
        case TOKEN_DOUBLE_COLON: return "::";
        case TOKEN_ARROW: return "->";
        case TOKEN_DOUBLE_DOT: return "..";
        case TOKEN_TRIPLE_DOT: return "...";
        case TOKEN_SHIFT_LEFT: return "<<";
        case TOKEN_SHIFT_RIGHT: return ">>";
        case TOKEN_PLUS_EQUAL: return "+=";
        case TOKEN_MINUS_EQUAL: return "-=";
        case TOKEN_STAR_EQUAL: return "*=";
        case TOKEN_SLASH_EQUAL: return "/=";
        case TOKEN_PERCENT_EQUAL: return "%=";
        case TOKEN_CARET_EQUAL: return "^=";
        case TOKEN_AMPERSAND_EQUAL: return "&=";
        case TOKEN_PIPE_EQUAL: return "|=";
        case TOKEN_TILDE_EQUAL: return "~=";
        case TOKEN_SHIFT_LEFT_EQUAL: return "<<=";
        case TOKEN_SHIFT_RIGHT_EQUAL: return ">>=";
        case TOKEN_PLUS_PLUS: return "++";
        case TOKEN_MINUS_MINUS: return "--";
        case TOKEN_QUESTION_QUESTION: return "??";
        case TOKEN_NEWLINE: return "NEWLINE";
        case TOKEN_EOF: return "EOF";
        case TOKEN_INVALID: return "INVALID";
        default: {
            static char buffer[2];
            buffer[0] = (char)type;
            buffer[1] = '\0';
            return buffer;
        }
    }
}

void print_token(Token token) {
    printf("Token: %s", token_type_to_string(token.type));
    if (token.type == TOKEN_IDENTIFIER || token.type == TOKEN_STRING || 
        token.type == TOKEN_NUMBER || token.type == TOKEN_CHARACTER) {
        printf(" ('");
        for (int i = 0; i < token.length; i++) {
            printf("%c", token.start[i]);
        }
        printf("')");
    }
    printf(" at line %d, column %d\n", token.line, token.column);
}