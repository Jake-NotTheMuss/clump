/*
** clump.c
** Generate a clump
*/

#include <stdio.h>
#include <string.h>

#include "clump.h"

static int cf_comp (const void *_a, const void *_b) {
  const ClumpFile *a = _a, *b = _b;
  return a->name < b->name ? -1 : 1;
}


void clump_add (Clump *clump, const char *filename) {
  FILE *f;
  STRT_String *ts;
  ClumpFile *cf;
  if (clump->nfiles >= ARRAY_COUNT(clump->files))
    THROW("cannot add another file to clump: max files reached (%d)",
          ARRAY_COUNT(clump->files));
  cf = &clump->files[clump->nfiles++];
  /* get file size */
  f = xopen(filename, "rb");
  cf->size = (u64)filesize(f, filename);
  fclose(f);
  /* get hash of file name */
  ts = (STRT_String *)strt_find(clump->table, filename);
  if (ts && filename_cmp(strt_getstr(ts), filename))
    THROW("hash table collision: '%s' and '%s'", (char *)ts->u.data, filename);
  ts = strt_newstr(clump->table, filename);
  cf->name = ts->hash;
  /* calculate section offset of file */
  ALIGNVAR(clump->pos, CLUMP_ALIGNMENT);
  if (clump->pos >> CLUMP_ALIGNMENT_POWER > UINT_MAX)
    THROW("section offset too large to hold in unsigned integer (%lx)",
          (unsigned long)(clump->pos >> CLUMP_ALIGNMENT_POWER));
  cf->sec = (unsigned int)(clump->pos >> CLUMP_ALIGNMENT_POWER);
  clump->pos += cf->size;
  if ((size_t)cf->size > clump->maxsize)
    clump->maxsize = (size_t)cf->size;
}

static void pad (Clump *clump, size_t n) {
  static const char buf [64];
  for (; n >= sizeof buf; n -= sizeof buf)
    xwrite(buf, sizeof buf, clump->f, clump->name);
  if (n)
    xwrite(buf, n, clump->f, clump->name);
}

void clump_write (Clump *clump) {
  /* sorted version of the array */
  ClumpFile scf [ARRAY_COUNT(clump->files)];
  char *buf;
  int i;
  assert(clump->f != NULL);
  if (clump->ninitial)
    xseek(clump->f, 0l, SEEK_SET, clump->name);
  /* create a sorted version of the files array to write */
  memcpy(scf, clump->files, sizeof(clump->files));
  qsort(scf, clump->nfiles, sizeof(ClumpFile), cf_comp);
  /* write file header */
  xwrite(scf, sizeof(clump->files), clump->f, clump->name);
  RELAY_ERROR();
  buf = xmalloc(clump->maxsize);
  TRY(
    if (clump->ninitial)
      xseek(clump->f, (long)clump->initialpos, SEEK_SET, clump->name);
    /* write each file entry */
    for (i = clump->ninitial; i < clump->nfiles; i++) {
      FILE *cur;  /* current file to add to clump */
      ClumpFile *cf = &clump->files[i];
      long pos; long newpos;
      const char *curname;
      const STRT_String *ts = strt_findh(clump->table, cf->name);
      curname = strt_getstr(ts);
      pos = xtell(clump->f, clump->name);
      newpos = (long)(cf->sec) << CLUMP_ALIGNMENT_POWER;
      assert(newpos >= pos);
      pad(clump, (size_t)(newpos - pos));
      cur = xopen(curname, "rb");
      TRY(xread(buf, cf->size, cur, curname));
      fclose(cur);
      RELAY_ERROR();
      xwrite(buf, cf->size, clump->f, clump->name);
    });
  free(buf);
  RELAY_ERROR();
  /* write hashmap */
  {
    long pos = xtell(clump->f, clump->name);
    ALIGNVAR(pos, CLUMP_ALIGNMENT);
    xseek(clump->f, pos, SEEK_SET, clump->name);
    char *buf;
    FOREACH_CLUMP_EX(scf, cf, {
      const STRT_String *ts = strt_findh(clump->table, cf->name);
      if (ts == NULL)
        THROW("%s: hash map incomplete", clump->name);
      buf = getbuffer(ts->len+1);
      normalize_path(buf, strt_getstr(ts), '\\');
      fprintf(clump->f, "%08x:%s\n", cf->name, buf);
    });
  }
}
