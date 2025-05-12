/*
** xmalloc.c
*/

#include "util.h"

void xerror (const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "%s: ", progname);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  abort();
}


void xthrow (const char *fmt, ...) {
  static char buf [0x400];
  va_list ap;
  va_start(ap, fmt);
  vsprintf(buf, fmt, ap);
  va_end(ap);
  g_errormsg = buf;
  longjmp(g_lj->b, 1);
}

void *xmalloc (size_t n) {
  return xrealloc(NULL, n);
}

void *xrealloc (void *mem, size_t n) {
  mem = realloc(mem, n);
  if (mem == NULL)
    xerror("cannot allocate %lu: (%s)", (unsigned long)n, strerror(errno));
  return mem;
}

char *xstrdup (const char *s) {
  char *s1 = strdup(s);
  if (s1 == NULL)
    xerror("cannot duplicate string: %s", strerror(errno));
  return s1;
}
