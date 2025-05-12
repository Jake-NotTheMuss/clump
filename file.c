/*
** file.c
** file operations
*/

#include "util.h"

FILE *xopen (const char *name, const char *mods) {
  FILE *f = fopen(name, mods);
  if (f == NULL)
    THROW("cannot open '%s': %s", name, strerror(errno));
  return f;
}

void xread (void *buf, size_t n, FILE *f, const char *name) {
  if (n != fread(buf, 1, n, f) || ferror(f)) {
    const char *serr = errno ? strerror(errno) : "file too short";
    THROW("%s: cannot read %lu bytes from file: %s", name, (unsigned long)n,
          serr);
  }
}

void xwrite (const void *buf, size_t n, FILE *f, const char *name) {
  if (n != fwrite(buf, 1, n, f) || ferror(f)) {
    THROW("%s: cannot write %lu bytes to file: %s", name, (unsigned long)n,
          strerror(errno));
  }

}

long xtell (FILE *f, const char *name) {
  long pos = ftell(f);
  if (pos == -1l)
    THROW("%s: cannot get file position: %s", name, strerror(errno));
  return pos;
}

void xseek (FILE *f, long pos, int origin, const char *name) {
  if (fseek(f, pos, origin) != 0)
    THROW("%s: cannot seek to file position: %s", name, strerror(errno));
}

unsigned long filesize (FILE *f, const char *name) {
  long size, pos = xtell(f, name);
  xseek(f, 0l, SEEK_END, name);
  size = xtell(f, name);
  xseek(f, pos, SEEK_SET, name);
  return (unsigned long)size;
}

int fileexists (const char *name) {
  FILE *f = fopen(name, "rb");
  if (f == NULL)
    return 0;
  else {
    fclose(f);
    return 1;
  }
}
