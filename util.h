/*
** util.h
*/

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef util_h
#define util_h

#if defined _WIN32
typedef unsigned __int64 u64;
#elif ULONG_MAX > 0xFFFFFFFFUL && ULONG_MAX == 0xFFFFFFFFFFFFFFFFUL
typedef unsigned long u64;
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <stdint.h>
typedef uint64_t u64;
#else
#error no 64-bit type found
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define ALIGN(x,n) (((x)+((n-1)))&~((n)-1))
#define ALIGNVAR(x,n) ((x) = ALIGN(x,n))

#define ARRAY_COUNT(a) ((int)(sizeof(a)/sizeof(a[0])))

struct longjump_s {
  struct longjump_s *prev;
  jmp_buf b;
};

extern struct longjump_s *g_lj;
extern char *g_errormsg;

#define TRY(x) do { \
  struct longjump_s _lj; \
  _lj.prev = g_lj; \
  g_lj = &_lj; \
  g_errormsg = NULL; \
  if (setjmp(_lj.b) == 0) { x; } \
  g_lj = _lj.prev; \
} while (0)


#define THROW (g_lj ? xthrow : xerror)
#define RELAY_ERROR() do { if (g_errormsg) THROW("%s", g_errormsg); } while (0)


extern const char *progname;
extern void xerror (const char *fmt, ...);
extern void *xmalloc (size_t n);
extern void *xrealloc (void *mem, size_t n);
extern char *xstrdup (const char *s);
extern void xthrow (const char *fmt, ...);

extern FILE *xopen (const char *name, const char *mods);
extern void xread (void *buf, size_t n, FILE *f, const char *name);
extern void xwrite (const void *buf, size_t n, FILE *f, const char *name);
extern long xtell (FILE *f, const char *name);
extern void xseek (FILE *f, long offs, int origin, const char *name);
extern unsigned long filesize (FILE *f, const char *name);
extern int fileexists (const char *name);

#if (defined(_WIN32) && !defined(__CYGWIN__)) || defined(__MSDOS__) || \
  defined(__DJGPP__) || defined(__OS2__)
# define HAVE_DOS_BASED_FILE_SYSTEM
# define IS_DIR_SEP(c)  ((c) == '/' || (c) == '\\')
#else
# undef HAVE_DOS_BASED_FILE_SYSTEM
# define IS_DIR_SEP(c)  ((c) == '/') 
#endif

extern void *getbuffer (size_t n);

extern void createpath (const char *path);
extern char *basename (const char *name);
extern int hasext (const char *name, const char *ext);
extern int filename_cmp (const char *a, const char *b);
extern void normalize_path (char *buf, const char *path, int sep);

extern int icmp (const char *s1, const char *s2);
extern int incmp (const char *s1, const char *s2, size_t n);

#endif /* util_h */
