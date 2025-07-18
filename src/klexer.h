#ifndef klex_h
#define klex_h

#include <limits.h>
#include "kobject.h"
#include "kzio.h"

#define FIRST_RESERVED	(UCHAR_MAX + 1)

enum RESERVED {
  /* terminal symbols denoted by reserved words */
  TK_IF = FIRST_RESERVED, TK_ELSE, TK_FOR, TK_WHILE, TK_BREAK, TK_CONTINUE,
  TK_RETURN, TK_PROC, TK_STRUCT, TK_ENUM, TK_UNION, TK_DEFER, TK_USING,
  TK_IMPORT, TK_PACKAGE, TK_TRUE, TK_FALSE, TK_NIL, TK_IN, TK_OR_RETURN,
  TK_OR_ELSE, TK_CAST, TK_AUTO_CAST, TK_TRANSMUTE, TK_DISTINCT, TK_OPAQUE,
  TK_WHERE, TK_SWITCH, TK_CASE, TK_FALLTHROUGH, TK_MAP, TK_CONTEXT,
  /* other terminal symbols */
  TK_COLON_COLON, TK_COLON_EQ, TK_ARROW, TK_DOT_DOT, TK_DOT_DOT_LT,
  TK_EQ, TK_NE, TK_LE, TK_GE, TK_AND_AND, TK_OR_OR, TK_SHL, TK_SHR,
  TK_EOS,
  TK_FLT, TK_INT, TK_NAME, TK_STRING
};

/* number of reserved words */
#define NUM_RESERVED	(cast_int(TK_CONTEXT-FIRST_RESERVED + 1))

typedef union {
  kat_Number r;
  kat_Integer i;
  TString *ts;
} SemInfo;  /* semantics information */

typedef struct Token {
  int token;
  SemInfo seminfo;
} Token;

/* state of the lexer */
typedef struct LexState {
  int current;  /* current character (charint) */
  int linenumber;  /* input line counter */
  int lastline;  /* line of last token 'consumed' */
  Token t;  /* current token */
  Token lookahead;  /* look ahead token */
  struct FuncState *fs;  /* current function (parser) */
  struct kat_State *K;
  ZIO *z;  /* input stream */
  Mbuffer *buff;  /* buffer for tokens */
  Table *h;  /* to avoid collection/reuse strings */
  TString *source;  /* current source name */
} LexState;

LUAI_FUNC void katX_init (kat_State *K);
LUAI_FUNC void katX_setinput (kat_State *K, LexState *ls, ZIO *z,
                              TString *source, int firstchar);
LUAI_FUNC TString *katX_newstring (LexState *ls, const char *str, size_t l);
LUAI_FUNC void katX_next (LexState *ls);
LUAI_FUNC int katX_lookahead (LexState *ls);
LUAI_FUNC l_noret katX_syntaxerror (LexState *ls, const char *s);
LUAI_FUNC const char *katX_token2str (LexState *ls, int token);

/* Token names array */
extern const char *const katX_tokens[];

#endif