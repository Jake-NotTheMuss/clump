/*
** unclump.c
** Extract files from a clump
*/

#include <limits.h>
#include <stdio.h>

#include "clump.h"

static const char *getname (const Clump *clump, const ClumpFile *cf) {
  const STRT_String *ts = strt_findh(clump->table, cf->name);
  return ts ? strt_getstr(ts) : NULL;
}

long get_hashmap_offs (const Clump *clump) {
  const ClumpFile *max_sec_cf = clump->files;
  long offs;
  FOREACH_CLUMP(clump, cf, if (cf->sec > max_sec_cf->sec) max_sec_cf = cf);
  offs = (long)(((long)max_sec_cf->sec << 12) + max_sec_cf->size);
  return ALIGN(offs, 0x1000);
}

void clump_loadhash (const Clump *clump) {
  long oldpos = xtell(clump->f, clump->name);
  long offs = get_hashmap_offs(clump);
  size_t hashmapsize = filesize(clump->f, clump->name) - offs;
  char *buf = getbuffer(hashmapsize+1);
  /* load hashmap text form */
  xseek(clump->f, offs, SEEK_SET, clump->name);
  xread(buf, hashmapsize, clump->f, clump->name);
  xseek(clump->f, oldpos, SEEK_SET, clump->name);
  buf[hashmapsize] = 0;
  /* parse it and add it to the hash table */
  char *s = buf, *t;
  while (*s && *s != '\n') {
    char *endptr;
    unsigned int hash = strtoul(s, &endptr, 16);
    if (*endptr != ':')
      THROW("invalid hash map at end of clump file");
    s = endptr+1;
    t = strchr(s, '\n');
    if (t) *t = '\0';
    else break;
    strt_maph(clump->table, hash, s);
    s = t+1;
  }
}

void unclump(const Clump *clump, ClumpFileWriter cb) {
  char namebuf [64];
  clump_loadhash(clump);
  FOREACH_CLUMP(clump, cf, {
    const char *name = getname(clump, cf);
    char *buf;
    long offs;
    assert(cf->sec << 12 <= LONG_MAX);
    offs = cf->sec << 12;
    /* use hash as the filename if not found in the hashtable */
    if (name == NULL) {
      sprintf(namebuf, "%x", cf->name);
      name = namebuf;
    }
    xseek(clump->f, offs, SEEK_SET, clump->name);
    buf = xmalloc(cf->size);
    TRY(xread(buf, cf->size, clump->f, clump->name);
        (*cb)(name, buf, cf->size);
       );
    free(buf);
    RELAY_ERROR();
  });
}

void clump_list (const Clump *clump, ClumpListWriter cb) {
  clump_loadhash(clump);
  FOREACH_CLUMP(clump, cf, {
    (*cb)(getname(clump, cf), cf);
  });
}

int clump_count (const Clump *clump) {
  int count = 0;
  FOREACH_CLUMP(clump, cf, count++);
  return count;
}
