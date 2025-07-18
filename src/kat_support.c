#include "minimal_headers.h"
#include "klex.h"

/* Simple string creation */
TString *katS_newlstr (kat_State *K, const char *str, size_t l) {
    (void)K;
    for (int i = 0; i < NUM_RESERVED; i++) {
      if (strlen(katX_tokens[i]) == l && memcmp(katX_tokens[i], str, l) == 0) {
        TString *ts = (TString *)malloc(sizeof(TString) + l);
        ts->extra = i + 1;
        ts->len = l;
        memcpy(ts->contents, str, l);
        ts->contents[l] = '\0';
        return ts;
      }
    }
    
    TString *ts = (TString *)malloc(sizeof(TString) + l);
    ts->extra = 0;
    ts->len = l;
    memcpy(ts->contents, str, l);
    ts->contents[l] = '\0';
    return ts;
  }

TString *katS_new (kat_State *K, const char *str) {
  return katS_newlstr(K, str, strlen(str));
}

/* Buffer management */
void katZ_resizebuffer (kat_State *K, Mbuffer *buff, size_t size) {
  (void)K;
  buff->buffer = (char *)realloc(buff->buffer, size);
  buff->buffsize = size;
  if (buff->n > size) buff->n = size;
}

/* ZIO functions */
int katZ_fill (ZIO *z) {
  size_t size;
  kat_State *K = z->K;
  const char *buff;
  buff = z->reader(K, z->data, &size);
  if (buff == NULL || size == 0)
    return EOZ;
  z->n = size - 1;  /* discount char being returned */
  z->p = buff;
  return cast_uchar(*(z->p++));
}

/* Simple memory allocator */
static void *default_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud; (void)osize;  /* not used */
  if (nsize == 0) {
    free(ptr);
    return NULL;
  }
  else
    return realloc(ptr, nsize);
}

/* State management */
kat_State *kat_newstate (kat_Alloc f, void *ud) {
  kat_State *K;
  if (f == NULL) {
    f = default_alloc;
    ud = NULL;
  }
  K = (kat_State *)f(ud, NULL, 0, sizeof(kat_State));
  if (K == NULL) return NULL;
  K->frealloc = f;
  K->ud = ud;
  K->buff.buffer = NULL;
  K->buff.n = 0;
  K->buff.buffsize = 0;
  return K;
}

void kat_close (kat_State *K) {
  if (K->buff.buffer)
    K->frealloc(K->ud, K->buff.buffer, K->buff.buffsize, 0);
  K->frealloc(K->ud, K, sizeof(kat_State), 0);
}