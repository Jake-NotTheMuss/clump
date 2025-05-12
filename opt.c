/*
** opt.c
** command line argument parser
*/

#include <stdarg.h>
#include <stddef.h>

#ifndef LIBOPT_SOLO
#include <stdio.h> /* fprintf */
#include <stdlib.h> /* exit */
#include <string.h> /* strchr, strcmp, strncmp, strlen */
#include <ctype.h> /* tolower */
#endif /* !LIBOPT_SOLO */

static int caseless;

void opt_caseless (int x) {
  caseless = x;
}

#ifndef LIBOPT_SOLO
#define L tolower
#else /* !LIBOPT_SOLO */
/* no 'tolower' */
#define L(x) x
/*
** provide strchr and strlen if in a freestanding environment
*/
static char *strchr (const char *s, int c) {
  char ch = c;
  for (; *s != ch; s++)
    if (*s == 0) return NULL;
  return (char *)s;
}

static size_t strlen (const char *s) {
  const char *s1;
  for (s1 = s; *s1; s1++) {}
  return (size_t)(s1 - s);
}
#endif /* LIBOPT_SOLO */

/* caseless strcmp and strncmp */
static int cmp (const char *s1, const char *s2) {
  for (; *s1 == *s2 || (caseless && L(*s1) == L(*s2)); s1++, s2++)
    if (*s1 == 0) return 0;
  return *(unsigned char *)s1 < *(unsigned char *)s2 ? -1 : 1;
}

static int ncmp (const char *s1, const char *s2, size_t n) {
  for (; n; s1++, s2++, n--) {
    if (*s1 != *s2 && (!caseless || L(*s1) != L(*s2)))
      return *(unsigned char *)s1 < *(unsigned char *)s2 ? -1 : 1;
    if (*s1 == 0) break;
  }
  return 0;
}

#include "opt.h"

/* print function */
#ifdef LIBOPT_SOLO

/* user-provided print function for error messages */
static void (*fn_print) (const char *, ...);

static void default_print (const char *fmt, ...) {
  (void)fmt;
}

#define opt_print (fn_print ? (*fn_print) : default_print)

void opt_setprintfn (void (*fn) (const char *, ...)) {
  fn_print = fn;
}

#else /* LIBOPT_SOLO */

static void opt_print (const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

#endif /* !LIBOPT_SOLO */

const char *opt_arg;

static const struct opt_s *progopts;

/* used by --help and --version handlers */
static const char *local_progname = "";
static const char *version = "0.0.0";

void opt_setversion (const char *str) {
  version = str ? str : "";
}

static const char *usage = "[options] arg...";

void opt_setusage (const char *str) {
  usage = str;
}

#define HAVE_ARG(o) ((o)->flags & OPT_HAVE_ARG)

/* number of characters preceding the description line */
#define DESC_INDENT 24

/* print the help message */
void opt_usage (void) {
  const struct opt_s *o;
  opt_print("Usage: %s", local_progname);
  if (usage && *usage) opt_print(" %s", usage);
  opt_print("\n\n");
  opt_print("Options:\n");
  for (o = progopts; o->s || o->l; o++) {
    int n = 0;
    const char *desc = o->desc ? o->desc : "";
    /* print short option */
    opt_print("  %s", o->s ? o->s : "  ");
    /* print long option */
    if (o->l) {
      opt_print("%c %s", o->s ? ',' : ' ', o->l);
      n += strlen(o->l) + 2;
    }
    if (HAVE_ARG(o)) {
      const char *v;
      int vlen;
      if (*desc == '[' && strchr(desc, ']')) {
        v = desc + 1;
        desc = strchr(v, ']') + 1; /* description starts after [...] */
        vlen = desc - 1 - v;
      }
      else
        v = "VALUE", vlen = (int)strlen(v);
      opt_print("=%.*s", vlen, v);
      n += vlen + 1;
    }
    /* add space before the description if needed */
    if (n < DESC_INDENT) opt_print("%*s", DESC_INDENT - n, "");
    opt_print("%s\n", desc);
  }
}

/* '--help' handler */
int opt_handle_help (void) {
  opt_usage();
#ifndef LIBOPT_SOLO
  exit(EXIT_SUCCESS);
#endif
  return LIBOPT_HELP;
}

/* '--version' handler */
int opt_handle_version (void) {
  opt_print("%s %s\n", local_progname, version);
  return 0;
}

static void check_opt (const struct opt_s *o, const char *arg) {
  if (o->value) {
    if (o->flags & OPT_SET_FLAG)
      *(int *)o->value = 1;
    else if (o->flags & OPT_CLR_FLAG)
      *(int *)o->value = 0;
    else if (o->flags & OPT_SET_VALUE)
      *(const char **)o->value = arg;
  }
}

#define err_arg_required(opt) \
  opt_print("%s: option requires an argument -- %s\n", local_progname, opt)


#define STREQ(a,b) (cmp(a, b) == 0)
#define STREQN(a,b,n) (ncmp(a,b,n) == 0)

/* handle all options from the command line, return the index of the first
   non-option argument, or (-1) if an error occurred */
int opt_parse (int argc, const void *_argv, const struct opt_s *options,
               const void *progname) {
  const char *const *argv = _argv;
  int i;
  if (options == NULL)
    return -1;
  progopts = options;
  /* set program name for the caller */
  if (argv[0] && argv[0][0]) {
    local_progname = argv[0];
    if (progname) *(const char **)progname = argv[0];
  }
  for (i = 1; i < argc; i++) {
    const struct opt_s *o;
    /* return value of handler (nonzero indicates error) */
    int res = 0;
    /* supplied value for an argument */
    opt_arg = NULL;
    if (argv[i][0] != '-')  /* non-option */
      return i;
    else if (cmp(argv[i], "--") == 0)  /* '--' indicates end of options */
      return i+1;
    /* parse a single short or long option; do this before parsing short
       options in case the host program uses a single '-' in long options */
    for (o = progopts; o->s || o->l; o++) {
      /* check for argument without supplied value or with supplied value in
         the next argument */
      int s = o->s && STREQ(argv[i], o->s);
      int l = o->l && STREQ(argv[i], o->l);
      if (s || l) {
        if (HAVE_ARG(o)) {
          if (i+1 >= argc) {
            err_arg_required(argv[i] + 1 + l);
            return -1;
          }
          opt_arg = argv[++i];
        }
        arghandler:
        check_opt(o, opt_arg);
        res = o->handler ? (*o->handler)() : 0;
        if (res != 0)
          return res;
        break;
      }
      /* check for a argument of the form --OPTION=VALUE */
      else if (HAVE_ARG(o)) {
        size_t sl = o->s ? strlen(o->s) : 0, ll = o->l ? strlen(o->l) : 0;
        if (o->s && STREQN(argv[i], o->s, sl) && argv[i][sl] == '=')
          opt_arg = argv[i] + sl + 1;
        else if (o->l && STREQN(argv[i], o->l, ll) && argv[i][ll] == '=')
          opt_arg = argv[i] + ll + 1;
        if (opt_arg)
          goto arghandler;
      }
    }
    if (o->s || o->l) continue; /* found matching option */
    /* parse multiple short options in one argument */
    if (argv[i][0] == '-' && argv[i][1] != '-') {
      /* example: '-abc' where a, b, and c are valid short options */
      /* once a short option that takes an argument is encountered, the rest of
         the string is considered the supplied value
         example: '-abcdef', where c takes an argument, is equivalent to
         '-a -b -c=def' */
      const char *s = &argv[i][1];
      for (; *s; s++) {
        for (o = progopts; o->s || o->l; o++) {
          if (o->s == NULL) continue;
          if (*s == o->s[1]) {
            if (HAVE_ARG(o)) {
              /* get the supplied value; either the rest of the string, or if
                 the string ends here, the next argument */
              s++;
              if (*s == 0) {
                if (i+1 >= argc) {
                  err_arg_required(o->s + 1);
                  return -1;
                }
                opt_arg = argv[++i];
              }
              else
                opt_arg = s + (*s == '=');
              goto arghandler;
            }
            check_opt(o, NULL);
            res = o->handler ? (*o->handler)() : 0;
            if (res)
              return res;
            break;
          }
        }
        if (!o->s && !o->l)
          goto unrecognized_option;
      }
      continue;
    }
    /* reached end of option list */
    unrecognized_option:
    opt_print("%s: unrecognized option: '%s'\n", local_progname, argv[i]);
    return -1;
  }
  return i;
}
