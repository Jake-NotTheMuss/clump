/*
** strt.c
** freestanding string table implementation
*/

#include <limits.h>

#include "strt.h"

#define tb_free(tb, p, size) ((*(tb)->alloc)((tb)->ud, p, size, 0))
#define tb_alloc(tb, size) ((*(tb)->alloc)((tb)->ud, NULL, 0, size))

static unsigned int strt_hash (const STRT *tb, const char *s, size_t l);

void strt_init (STRT *tb, STRT_Alloc alloc, STRT_TooBig toobig, void *ud) {
  tb->hash = NULL;
  tb->nuse = tb->size = 0;
  tb->alloc = alloc;
  tb->toobig = toobig;
  tb->ud = ud;
  tb->hashfn = strt_hash;
  strt_resize(tb, 32);
}

#define FOREACH_NODE(list, var, action) do { \
  STRT_String *var = (list); \
  while (var) { STRT_String *_next = var->next; action; var = _next; } \
} while (0)

void strt_free (STRT *tb) {
  unsigned long i;
  for (i = 0; i < tb->size; i++)
    FOREACH_NODE(tb->hash[i], p, tb_free(tb, p, sizeof(*p) + p->len + 1));
  tb_free(tb, tb->hash, tb->size * sizeof(STRT_String *));
  tb->hash = NULL;
  tb->nuse = tb->size = 0;
}

void strt_resize (STRT *tb, unsigned long newsize) {
  STRT_String **newhash = tb_alloc(tb, newsize * sizeof(STRT_String *));
  unsigned long i;
  for (i=0; i<newsize; i++) newhash[i] = NULL;
  /* rehash */
  for (i=0; i<tb->size; i++)
    FOREACH_NODE(tb->hash[i], p,
                 unsigned int h = p->hash;
                 unsigned int h1 = h % newsize;  /* new position */
                 p->next = newhash[h1];  /* chain it */
                 newhash[h1] = p;
                );
  tb_free(tb, tb->hash, tb->size * sizeof(STRT_String *));
  tb->size = newsize;
  tb->hash = newhash;
}

static STRT_String *allocstr (STRT *tb, const char *str, size_t l,
                              unsigned int h) {
  static const STRT_String dummy;
  STRT_String *ts;
  size_t i;
  if (l+1 > (ULONG_MAX - sizeof(STRT_String)))
    (*tb->toobig)(tb->ud);
  ts = tb_alloc(tb, l+1+sizeof(STRT_String));
  if (ts == NULL) return NULL;
  *ts = dummy;  /* initialize extra fields added by the user */
  ts->len = l;
  ts->hash = h;
  /* copy string to TS */
  for (i = 0; i < l; i++)
    ((char *)(ts+1))[i] = str[i];
  ((char *)(ts+1))[i] = '\0';  /* ending 0 */
  h %= tb->size;
  ts->next = tb->hash[h];  /* chain new entry */
  tb->hash[h] = ts;
  tb->nuse++;
  if (tb->nuse > tb->size && tb->size <= ULONG_MAX/2)
    strt_resize(tb, tb->size*2);  /* too crowded */
  return ts;
}

static int cmp (const void *p1, const void *p2, size_t n) {
  const unsigned char *s1 = p1, *s2 = p2;
  for (; n; n--, s1++, s2++) {
    int c1 = *s1, c2 = *s2;
    if (c1 != c2) return (c1 < c2) ? -1 : 1;
  }
  return 0;
}

static unsigned int strt_hash (const STRT *tb, const char *s, size_t l) {
  unsigned int h = (unsigned int)l;  /* seed */
  size_t step = (l>>5)+1; /* if string is too long, don't hash all its chars */
  size_t l1;
  for (l1=l; l1>=step; l1-=step)
    h = h ^ (((h<<5)+(h>>2))+(unsigned char)(s[l1-1]));
  return h;
}

static const STRT_String *getstr (STRT *tb, const char *str, size_t l,
                                  int create) {
  const STRT_String *ts;
  unsigned int h = (*tb->hashfn)(tb, str, l);
  for (ts = tb->hash[h % tb->size]; ts != NULL; ts = ts->next) {
    if (ts->len == l && (cmp(str, strt_getstr(ts), l) == 0))
      return ts;
  }
  return create ? allocstr(tb, str, l, h) : NULL;
}

STRT_String *strt_newlstr (STRT *tb, const char *str, size_t l) {
  return (STRT_String *)getstr(tb, str, l, 1);
}

STRT_String *strt_newstr (STRT *tb, const char *str) {
  size_t l = 0;
  while (str[l]) l++;
  return strt_newlstr(tb, str, l);
}

/* map a string to a specific hash value */
STRT_String *strt_maph (STRT *tb, unsigned int h, const char *str) {
  size_t l = 0;
  while (str[l]) l++;
  return allocstr(tb, str, l, h);
}

const STRT_String *strt_find (const STRT *tb, const char *str) {
  size_t l = 0;
  while (str[l]) l++;
  return getstr((STRT *)tb, str, l, 0);
}

const STRT_String *strt_findh (const STRT *tb, unsigned int h) {
  const STRT_String *ts;
  for (ts = tb->hash[h % tb->size]; ts != NULL; ts = ts->next) {
    if (ts->hash == h)
      return ts;
  }
  return NULL;
}

#if 0
void strt_print (const STRT *tb) {
  int i;
  extern int (printf) (const char *, ...);
  printf("Table size: %lu\n", tb->size);
  for (i = 0; i < tb->size; i++)
    FOREACH_NODE(tb->hash[i], p, printf("%8x: %s\n", p->hash, strt_getstr(p)));
}
#endif
