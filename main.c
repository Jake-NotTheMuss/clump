/*
** main.c
*/

#include "util.h"

#include "hash.h"
#include "strt.h"
#include "opt.h"

/* xmalloc stuff */
const char *progname = "clump";

struct longjump_s *g_lj = NULL;
char *g_errormsg = NULL;

/* option variables */
static int list = 0;
static int count;
static int append = -1;
/* 0 = unclump, 1 = clump */
static int clump = -1;
static const char *outdir = ".";
static const char *output;

/* memory */
static struct {
  char *b;
  size_t n;
} buffer;

static STRT hashtable;

void *getbuffer (size_t n) {
  if (n > buffer.n) {
    buffer.n = (n + 63) & ~63;
    buffer.b = xrealloc(buffer.b, buffer.n);
  }
  return buffer.b;
}

static unsigned int hashname (const char *name) {
  unsigned int i, len = (unsigned int)strlen(name);
  char *buf = getbuffer(len+1);
  for (i = 0; i < len; i++) {
    int c = tolower(name[i]);
    if (c == '\\') c = '/';
    buf[i] = c;
  }
  buf[i] = 0;
  return hash((void *)name, len, 0);
}

static unsigned int ht_hash (const STRT *tb, const char *s, size_t l) {
  return hashname(s);
  (void)tb; (void)l;
}

static void *ht_alloc (void *ud, void *mem, size_t oldn, size_t newn) {
  if (newn)
    mem = xrealloc(mem, newn);
  else
    free(mem), mem = NULL;
  (void)ud; (void)oldn;
  return mem;
}

static void ht_toobig (void *ud) {
  xerror("memory allocation error: block too big");
  (void)ud;
}

/* this will have the subdirectory name for the current input clump file within
   OUTDIR */
static char subdir [64] = {0};

#include "clump.h"

/* writer callback for a single clump file entry */
static void unclump_writer (const char *name, void *buf, size_t size) {
  int err;
  size_t subdirlen = strlen(subdir);
  size_t outdirlen = strlen(outdir);
  char *path = getbuffer(outdirlen+1 + subdirlen+1 + strlen(name)+1);
  FILE *out;
  memcpy(path, outdir, outdirlen);
  path[outdirlen] = '/';
  if (*subdir) {
    memcpy(path + outdirlen + 1, subdir, subdirlen);
    path[outdirlen+1+subdirlen] = '/';
    subdirlen++;
  }
  strcpy(path + outdirlen + 1 + subdirlen, name);
  createpath(path);
  out = xopen(path, "wb");
  err = (fwrite(buf, 1, size, out) != size || ferror(out));
  fclose(out);
  if (err) {
    THROW("cannot write %lu bytes to file %s: %s", (unsigned long)size, name,
          strerror(errno));
  }
}

static void unclump_list (const char *name, const ClumpFile *cf) {
  printf("%8x    %s\n", cf->name, name ? name : "");
}

static Clump outclump;

static int finalize_clump (void) {
  outclump.f = xopen(outclump.name, append ? "rb+" : "wb");
  TRY(clump_write(&outclump));
  fclose(outclump.f);
  if (g_errormsg)
    fprintf(stderr, "%s\n", g_errormsg);
  return g_errormsg != NULL;
}

static void read_header (Clump *clump) {
  /* read all clump file entries in the header */
  if (fread(clump->files, 1, sizeof(clump->files), clump->f)
      != sizeof(clump->files) || ferror(clump->f)) {
    fclose(clump->f);
    errno
    ? xerror("%s: cannot read clump header: %s", clump->name, strerror(errno))
    : xerror("%s: file too short (may not be a clump file)", clump->name);
  }
}

static int dofile (const char *filename) {
  if (clump) {
    /* generate clump */
    if (outclump.name == NULL) {
      const char *outname = output ? output : "out.clump";
      outclump.name = outname;
      outclump.table = &hashtable;
      outclump.pos = sizeof(outclump.files);
      if (append) {
        TRY(outclump.f = xopen(outname, "rb"));
        append = outclump.f != NULL;
      }
      if (append) {
        outclump.f = xopen(outname, "rb");
        read_header(&outclump);
        outclump.pos = get_hashmap_offs(&outclump);
        outclump.nfiles = clump_count(&outclump);
        clump_loadhash(&outclump);
        fclose(outclump.f);
        outclump.f = NULL;
      }
      outclump.ninitial = outclump.nfiles;
      outclump.initialpos = outclump.pos;
    }
    TRY(clump_add(&outclump, filename));
  }
  else {
    /* unclump file */
    Clump clump;
    const char *name = basename(filename);
    size_t len = strlen(name);
    clump.name = filename;
    clump.f = xopen(filename, "rb");
    clump.table = &hashtable;
    errno = 0;
    read_header(&clump);
    if (list || count) {
      if (list)
        clump_list(&clump, unclump_list);
      if (count)
        printf("Total files in %s: %d\n", filename, clump_count(&clump));
    }
    else {
      /* generate a subdirectory name for the extracted files */
      if (hasext(name, ".clump"))
        len -= sizeof(".clump")-1;
      if (len >= sizeof(subdir))
        len = sizeof(subdir) - 1;
      memcpy(subdir, name, len);
      subdir[len] = 0;
      fprintf(stderr, "Extracting %s to '%s/%s'\n", filename, outdir, subdir);
      TRY(unclump(&clump, unclump_writer));
    }
    fclose(clump.f);
  }
  if (g_errormsg)
    fprintf(stderr, "%s\n", g_errormsg);
  return g_errormsg != NULL;
}

/* program options */
static const struct opt_s progopts [] = {
  OPT_HELP, OPT_VERSION,
  { "-a", NULL, "Add to existing clump file", OPT_SET_FLAG, &append, NULL },
  { "-c", NULL, "Create a clump file from input files", OPT_SET_FLAG, &clump,
    NULL },
  { "-d", NULL, "[DIR]Extract clump to directory DIR (-u)", OPT_SET_VALUE,
    &outdir, NULL },
  { "-l", NULL, "List files only", OPT_SET_FLAG, &list, NULL },
  { "-n", NULL, "Count files only", OPT_SET_FLAG, &count, NULL },
  { "-o", NULL, "[FILE]Set name of output clump file (-c)",
    OPT_SET_VALUE, &output, NULL },
  { "-u", NULL, "Extract from input clump files", OPT_CLR_FLAG, &clump, NULL },
  { NULL }
};

int main (int argc, const char **argv) {
  int i, error = 0;
  strt_init(&hashtable, ht_alloc, ht_toobig, NULL);
  hashtable.hashfn = ht_hash;
  /* parse options */
  opt_setusage("[options] file...");
  opt_setversion("0.0.0");
  i = opt_parse(argc, argv, progopts, &progname);
  if (i < 0 || i >= argc) { /* error parsing options or no input files */
    if (i >= argc) opt_usage();
    return EXIT_FAILURE;
  }
  if (append > 0) {
    if (clump == 0) {
      fprintf(stderr, "Error: '-a' and '-u' are not compatible\n");
      return EXIT_FAILURE;
    }
    clump = 1;
  }
  if (clump == -1) {
    int nfiles = argc - i;
    /* not specified in options, infer from input file */
    clump = !(hasext(argv[i], ".clump"));
    if (nfiles > 1 && clump != !hasext(argv[i+1], ".clump")) {
      fprintf(stderr, "Warning: some input files have the '.clump' extension,"
              " but not all. Treating all input files as %s files.\n"
              "To disable this warning, explicitly provide either -c or -u.\n"
              "See --help for more details on program usage\n",
              clump ? "clump entry" : "clump archive");
    }
  }
  if (append == -1) append = 0;
  if (append && output == NULL) {
    fprintf(stderr, "You must provide an out file with '-a'\n");
    return EXIT_FAILURE;
  }
  /* process input files */
  for (; i < argc; i++)
    error |= dofile(argv[i]);
  if (clump)
    error |= finalize_clump();
  strt_free(&hashtable);
  free(buffer.b);
  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
