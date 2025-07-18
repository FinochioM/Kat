#include <stdio.h>
#include <string.h>
#include "minimal_headers.h"
#include "klex.h"
#include "kparser.h"

/* Reader function for string input */
typedef struct StringReader {
  const char *str;
  size_t size;
} StringReader;

static const char *string_reader(kat_State *K, void *ud, size_t *sz) {
  StringReader *sr = (StringReader *)ud;
  (void)K;  /* not used */
  if (sr->size == 0) return NULL;
  *sz = sr->size;
  sr->size = 0;  /* only read once */
  return sr->str;
}

void test_lexer(const char *code) {
  kat_State *K = kat_newstate(NULL, NULL);
  LexState ls;
  ZIO z;
  StringReader sr;
  TString *source;
  
  printf("\n=== Testing code: ===\n%s\n", code);
  printf("=== Tokens: ===\n");
  
  /* Initialize */
  katX_init(K);
  
  /* Setup string reader */
  sr.str = code;
  sr.size = strlen(code);
  
  /* Setup ZIO */
  z.K = K;
  z.reader = string_reader;
  z.data = &sr;
  z.n = 0;
  z.p = NULL;
  
  /* Create source name */
  source = katS_new(K, "test");
  
  /* Initialize lexer */
  katX_setinput(K, &ls, &z, source, ' ');
  
  /* Read and print tokens */
  do {
    katX_next(&ls);
    int token = ls.t.token;
    
    if (token >= FIRST_RESERVED && token < TK_EOS) {
      printf("RESERVED: %s\n", katX_token2str(&ls, token));
    } else if (token == TK_NAME) {
      printf("NAME: %.*s\n", (int)ls.t.seminfo.ts->len, ls.t.seminfo.ts->contents);
    } else if (token == TK_STRING) {
      printf("STRING: \"%.*s\"\n", (int)ls.t.seminfo.ts->len, ls.t.seminfo.ts->contents);
    } else if (token == TK_INT) {
      printf("INTEGER: %lld\n", (long long)ls.t.seminfo.i);
    } else if (token == TK_FLT) {
      printf("FLOAT: %g\n", ls.t.seminfo.r);
    } else if (token == TK_EOS) {
      printf("END OF SOURCE\n");
    } else if (token < 256) {
      printf("CHAR: '%c'\n", token);
    } else {
      printf("UNKNOWN TOKEN: %d\n", token);
    }
  } while (ls.t.token != TK_EOS);
  
  kat_close(K);
}

void test_parser(const char *code) {
  kat_State *K = kat_newstate(NULL, NULL);
  ZIO z;
  StringReader sr;
  TString *source;
  ASTNode *ast;
  
  printf("\n=== Parsing code: ===\n%s\n", code);
  
  /* Initialize */
  katX_init(K);
  
  /* Setup string reader */
  sr.str = code;
  sr.size = strlen(code);
  
  /* Setup ZIO */
  z.K = K;
  z.reader = string_reader;
  z.data = &sr;
  z.n = 0;
  z.p = NULL;
  
  /* Create source name */
  source = katS_new(K, "test");
  
  /* Parse */
  ast = katY_parser(K, &z, source, ' ');
  
  /* Clean up */
  katY_freeAST(K, ast);
  kat_close(K);
}

int main() {
  printf("Kat Language Test\n");
  printf("=================\n");
  
  printf("\n--- LEXER TESTS ---\n");
  
  /* Test basic tokens */
  test_lexer("main :: proc() {\n    x := 42\n    y : int = 10\n}");
  
  /* Test strings and operators */
  test_lexer("name := \"hello world\"\nif x == y && z != 0 {\n    return true\n}");
  
  printf("\n--- PARSER TESTS ---\n");
  
  /* Test variable declarations */
  test_parser("x := 42\ny : int = 10\nz :: 3.14");
  
  /* Test procedure */
  test_parser("main :: proc() {\n    x := 42\n    return x\n}");
  
  /* Test control flow */
  test_parser("if x > 0 {\n    y := 1\n} else {\n    y := 0\n}");
  
  /* Test loops */
  test_parser("for i in items {\n    print(i)\n}");
  
  return 0;
}