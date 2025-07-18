#ifndef kat_h
#define kat_h

#include <stddef.h>
#include <stdint.h>

typedef struct kat_State kat_State;

/*
** basic types
*/
#define KAT_TNONE		(-1)
#define KAT_TNIL		0
#define KAT_TBOOLEAN		1
#define KAT_TNUMBER		3
#define KAT_TSTRING		4

typedef double kat_Number;
typedef int64_t kat_Integer;

/*
** Type for memory-allocation functions
*/
typedef void * (*kat_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);

/*
** Type for functions that read/write blocks when loading/dumping Kat chunks
*/
typedef const char * (*kat_Reader) (kat_State *K, void *ud, size_t *sz);
typedef int (*kat_Writer) (kat_State *K, const void *p, size_t sz, void *ud);

/*
** Error codes
*/
#define KAT_OK		0
#define KAT_YIELD	1
#define KAT_ERRRUN	2
#define KAT_ERRSYNTAX	3
#define KAT_ERRMEM	4
#define KAT_ERRERR	5

/* Basic functions */
kat_State *kat_newstate (kat_Alloc f, void *ud);
void kat_close (kat_State *K);

/* Error handling */
#define l_noret		void

#endif