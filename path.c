/*
** path.c
*/

#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"

#ifdef _WIN32
#include <direct.h>
# ifndef mkdir
#  define mkdir(path, mode) _mkdir(path)
# endif
# ifndef stat
#  define stat _stat
# endif
# ifndef S_ISDIR
#  define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
# endif
#endif

int sys_mkdir (const char *path) {
  int rc = mkdir(path, 0777);
  if (rc && errno == EEXIST) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) rc = 0;
  }
  return rc;
}

void createpath (const char *path) {
  char *s, *buf = xstrdup(path);
  TRY(
  for (s = buf; *s; s++) {
    if (IS_DIR_SEP(*s)) {
      int c = *s;
      *s = 0;
      if (sys_mkdir(buf))
        THROW("cannot create path '%s': %s", buf, strerror(errno));
      *s = c;
    }
  }
  );
  free(buf);
  RELAY_ERROR();
}

char *basename (const char *name) {
  const char *base;
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
  if (isalpha(name[0]) && name[1] == ':')
    name += 2;
#endif
  for (base = name; *name; name++)
    if (IS_DIR_SEP(*name)) base = name + 1;
  return (char *)base;
}

int hasext (const char *name, const char *ext) {
  size_t len = strlen(name), extlen = strlen(ext);
  return (len > extlen && icmp(name + len - extlen, ext) == 0);
}

#define pathchar_eq(a,b) \
  (tolower(a) == tolower(b) || (IS_DIR_SEP(a) && IS_DIR_SEP(b)))

int filename_cmp (const char *a, const char *b) {
  for (; pathchar_eq(*a, *b); a++, b++)
    if (*a == 0) return 0;
  return 1;
}

void normalize_path (char *buf, const char *path, int sep) {
  for (; *path; buf++, path++) {
    int c = tolower(*path);
    if (IS_DIR_SEP(c)) c = sep;
    *buf = c;
  }
  *buf = 0;
}
