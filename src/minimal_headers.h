#ifndef minimal_headers_h
#define minimal_headers_h

#include "kat.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* kobject.h minimal */
typedef struct TString TString;
typedef struct Table Table;

#define EOZ	(-1)			/* end of stream */

#define cast_int(i)	((int)(i))
#define cast_char(i)	((char)(i))
#define cast_uchar(c)	((unsigned char)(c))

/* Common header for all collectible objects */
typedef struct GCObject GCObject;

struct TString {
  GCObject *next; int tt; unsigned char marked;
  unsigned char extra;  /* reserved words for lexer */
  unsigned int hash;
  size_t len;  /* number of characters in string */
  char contents[1];  /* actually variable sized */
};

/* kzio.h minimal */
typedef struct Zio ZIO;

typedef struct Mbuffer {
  char *buffer;
  size_t n;
  size_t buffsize;
} Mbuffer;

struct Zio {
  size_t n;			/* bytes still unread */
  const char *p;		/* current position in buffer */
  kat_Reader reader;		/* reader function */
  void *data;			/* additional data */
  kat_State *K;			/* Kat state (for reader) */
};

#define zgetc(z)  (((z)->n--)>0 ?  cast_uchar(*(z)->p++) : katZ_fill(z))

#define katZ_buffer(buff)	((buff)->buffer)
#define katZ_sizebuffer(buff)	((buff)->buffsize)
#define katZ_bufflen(buff)	((buff)->n)
#define katZ_buffremove(buff,i)	((buff)->n -= (i))
#define katZ_resetbuffer(buff) ((buff)->n = 0)

/* Basic string functions */
TString *katS_new (kat_State *K, const char *str);
TString *katS_newlstr (kat_State *K, const char *str, size_t l);

/* Basic table functions */
struct Table {
  GCObject *next; int tt; unsigned char marked;
  /* simplified table structure */
  TString **hash;
  unsigned int sizearray;
};

/* Buffer functions */
void katZ_resizebuffer (kat_State *K, Mbuffer *buff, size_t size);
int katZ_fill (ZIO *z);

/* Character classification */
#define lisspace(c)	(isspace(c))
#define lisdigit(c)	(isdigit(c))
#define lisalpha(c)	(isalpha(c))
#define lisalnum(c)	(isalnum(c))
#define lisprint(c)	(isprint(c))
#define lislalpha(c)	(isalpha(c) || (c) == '_')
#define lislalnum(c)	(isalnum(c) || (c) == '_')

/* Memory management */
#define MAX_SIZE		((~(size_t)0)-2)

/* Simple state structure */
struct kat_State {
  kat_Alloc frealloc;  /* function to reallocate memory */
  void *ud;         /* auxiliary data to 'frealloc' */
  Mbuffer buff;     /* temporary buffer */
};

/* Utility macros */
#define LUAI_FUNC
#define isreserved(s)	((s)->extra > 0)

/* Error handling */
#define katG_addinfo(K, msg, src, line) (msg)
#define katD_throw(K, errcode) exit(errcode)

/* Basic types */
typedef unsigned char lu_byte;

#endif