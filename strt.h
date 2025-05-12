/*
** strt.h
** freestanding string table interface
*/

#include <stddef.h>

#ifndef strt_h
#define strt_h

/*
** these functions will use the same userdata passed to strt_init
*/

/* allocator function to use for free, allocating, and reallocating; if
   allocating, the function is expected to return a non-NULL pointer */
typedef void *(*STRT_Alloc) (void *ud, void *mem, size_t oldn, size_t newn);
/* panic function when a string is too large to be allocated; if this function
   returns, then STRT will return a NULL pointer */
typedef void (*STRT_TooBig) (void *ud);

/* a node in the string table */
typedef struct STRT_String {
  struct STRT_String *next;
  size_t len;
  unsigned int hash;
  union {
    void *data;
    unsigned long reserved;
  } u;
} STRT_String;

/* returns the string part of a STRT_String */
#define strt_getstr(s) (const char *)((s) + 1)

typedef struct STRT STRT;

/* string table structure */
struct STRT {
  STRT_String **hash;
  unsigned long nuse;
  unsigned long size;
  void *ud;
  STRT_Alloc alloc;
  STRT_TooBig toobig;
  unsigned int (*hashfn) (const STRT *tb, const char *s, size_t l);
};

#define STRT_API extern

/* API */

STRT_API void strt_init (STRT *tb, STRT_Alloc alloc, STRT_TooBig toobig,
                         void *ud);
STRT_API void strt_free (STRT *tb);
STRT_API STRT_String *strt_newlstr (STRT *tb, const char *str, size_t l);
STRT_API STRT_String *strt_newstr (STRT *tb, const char *str);
/* map a string to a specific hash value */
STRT_API STRT_String *strt_maph (STRT *tb, unsigned int h, const char *str);

STRT_API const STRT_String *strt_find (const STRT *tb, const char *str);
STRT_API const STRT_String *strt_findh (const STRT *tb, unsigned int h);

STRT_API void strt_resize (STRT *tb, unsigned long newsize);

#if 0
STRT_API void strt_print (const STRT *tb);
#endif

#endif /* strt_h */
