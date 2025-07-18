#include <stdio.h>
#include <string.h>
#include "minimal_headers.h"
#include "klex.h"

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
  
  katX_init(K);
  
  sr.str = code;
  sr.size = strlen(code);
  
  z.K = K;
  z.reader = string_reader;
  z.data = &sr;
  z.n = 0;
  z.p = NULL;
  
  source = katS_new(K, "test");
  
  katX_setinput(K, &ls, &z, source, ' ');
  
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

int main() {
  printf("Kat Language Lexer Test\n");
  printf("=======================\n");
  
  test_lexer("main :: proc() {\n    x := 42\n    y : int = 10\n}");
  
  test_lexer("name := \"hello world\"\nif x == y && z != 0 {\n    return true\n}");
  
  test_lexer("// This is a comment\npi := 3.14159\ncount := 100\n/* block comment */");
  
  test_lexer("x := y -> z\na :: distinct int\nb := cast(int) c");
  
  return 0;
}