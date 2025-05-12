/*
** clump.h
** Clump file structures
*/

#include <stdio.h>

#ifndef clump_h
#define clump_h

#include "util.h"
#include "strt.h"

typedef struct {
  unsigned int name;  /* file name hash */
  unsigned int sec;  /* section offset */
  u64 size;  /* file size */
} ClumpFile;

#define CLUMP_MAX_FILES 2048

typedef struct {
  const char *name;  /* clump file name */
  FILE *f;
  STRT *table;  /* hash table for file names */
  ClumpFile files [CLUMP_MAX_FILES];
  /* the following fields are used by clump.c */
  int ninitial;
  int nfiles;
  u64 pos, initialpos;
  size_t maxsize;  /* max file size of all clump entries */
} Clump;

typedef void (*ClumpFileWriter) (const char *name, void *buf, size_t size);
typedef void (*ClumpListWriter) (const char *name, const ClumpFile *cf);

extern void unclump (const Clump *clump, ClumpFileWriter cb);
extern void clump_list (const Clump *clump, ClumpListWriter cb);
extern int clump_count (const Clump *clump);
extern void clump_loadhash (const Clump *clump);
extern long get_hashmap_offs (const Clump *clump);

extern void clump_add (Clump *clump, const char *filename);
extern void clump_write (Clump *clump);

#define FOREACH_CLUMP(clump, var, x) FOREACH_CLUMP_EX((clump)->files, var, x)

#define FOREACH_CLUMP_EX(arr, var, x) do { \
  int _i; \
  const ClumpFile *var = (arr); \
  for (_i = 0; var->name && _i < ARRAY_COUNT(arr); _i++, var++) { \
    x; \
  } \
} while (0)

#endif /* clump_h */
