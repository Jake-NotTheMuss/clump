/*
** opt.h
** API for libopt command line option parser
*/

/*
LIBOPT DOCUMENTATION
-------------------------------------------------------------------------------
Optional macros defined by builder:
  LIBOPT_SOLO (if in a freestanding environment)

Optional macros defined by program:
  OPT_HELP_SHORT_NAME (string literal: short version of '--help' option)
  OPT_VERSION_SHORT_NAME (string literal: short version of '--version' option)

Symbols defined by this library:
  Macros
    OPT_NO_ARG
    OPT_HAVE_ARG
    OPT_SET_FLAG
    OPT_CLR_FLAG
    OPT_SET_VALUE
    OPT_HELP
    OPT_VERSION
    LIBOPT_ERROR
    LIBOPT_HELP
  Types
    struct opt_s
  Functions
    opt_handle_help
    opt_handle_version
    opt_parse
    opt_setversion
    opt_usage
    opt_setusage
    opt_setprintfn
    opt_caseless
  Data
    opt_arg

How to use this software:
1. Include the header 'opt.h' in your program
2. Define an array variable of 'struct opt_s', terminated by a NULL-padded
   struct opt_s element:

    static int list = 0;

    const struct opt_s progopts [] = {
      OPT_HELP, OPT_VERSION,
      { "-l", "--list", "List files only", OPT_SET_FLAG, &list, NULL },
      { NULL }
    };

3. Call 'opt_parse':

    const char *progname = NULL;

    int main (int argc, char **argv) {
      int arg = opt_parse(argc, argv, progopts, &progname);
      if (arg < 0)
        return EXIT_FAILURE;
      return EXIT_SUCCESS;
    }

   Now, the variable 'arg' holds the index of the first non-option argument

About LIBOPT_SOLO:

If built with LIBOPT_SOLO defined, you should call 'opt_setprintfn' with your
print function before calling other functions.

About option handlers:

Option argument handlers are of the following prototype:
    int (arg_handler) (void);

The handler function may read the global variable 'opt_arg' to access the
supplied value for the option, if applicable. If the option takes a supplied
value, 'opt_arg' is guaranteed to be non-NULL.

The return code of the handler is zero if option parsing should continue as
normal. A negative return code tells 'opt_parse' to stop handling options and
return the code. An error code should never be positive, as 'opt_parse' returns
a positive integer only if no error occurs.

The following return codes have predefined meaning:
  LIBOPT_ERROR - indicates an error
  LIBOPT_HELP - indicates '--help' was supplied

Return codes (-1) to (-99) are reserved by this library. The user may define
their own codes at (-100) or lower.

About 'struct opt_s':

The description string (member 'desc') may be prefixed by
square-bracket-enclosed text that is used by the help handler
'opt_handle_help': The text inside the brackets is used as a variable name
for an option which takes a value, and the actual dsecription will start after
the close-bracket. Example:

    static const char *output;

    const struct opt_s progopts [] = {
      OPT_HELP,
      { "-o", "--output", "[FILE]Write output to FILE", OPT_ARG_SET_VALUE,
        &output, NULL },
      { NULL }
    };

Now, when invoking the program with --help, the help line for '--output' will
look like this:
  -o, --output=FILE       Write output to FILE
-------------------------------------------------------------------------------
END OF LIBOPT DOCUMENTATION
*/

#ifndef opt_h
#define opt_h

struct opt_s {
  const char *s, *l; /* short and long forms */
  const char *desc; /* description */
  int flags; /* 0 if the option takes no supplied value */
  void *value;  /* pointer to a variable, either 'int' or 'const char *' */
  int (*handler) (void);
};

/* flags for struct opt_s */
#define OPT_NO_ARG 0
#define OPT_HAVE_ARG 1  /* option requires an argument, e.g. --opt=arg */
#define OPT_SET_FLAG 2  /* set an integer variable pointed to by VALUE */
#define OPT_CLR_FLAG 4  /* clear an integer variable pointed to by VALUE */
#define OPT_SET_VALUE 9 /* set a string variable pointed to by VALUE */

/* the user may define these to "-h" and "-v" */
#ifndef OPT_HELP_SHORT_NAME
#define OPT_HELP_SHORT_NAME NULL
#endif
#ifndef OPT_VERSION_SHORT_NAME
#define OPT_VERSION_SHORT_NAME NULL
#endif

#define OPT_HELP \
  { OPT_HELP_SHORT_NAME, "--help", "Print this message and exit", OPT_NO_ARG, \
    NULL, opt_handle_help }
#define OPT_VERSION \
  { OPT_VERSION_SHORT_NAME, "--version", "Show version information and exit", \
    OPT_NO_ARG, NULL, opt_handle_version }

/* return codes for handlers */
#define LIBOPT_ERROR (-1)
#define LIBOPT_HELP (-2)

/* set by opt.c, to be utilized by handlers for options that take values */
extern const char *opt_arg;

/* standard option handlers, utilized by adding OPT_HELP and OPT_VERSION to
   the array of struct opt_s */
extern int opt_handle_help (void);
extern int opt_handle_version (void);

/*
** opt_parse(): argument parser
**  - returns the index of the first non-option argument in ARGV or a negative
**    integer indicating an error
**  - param 'options' must point to an array of struct opt_s, terminated by a
**    NULL-padded element
**  - param 'progname' may be NULL or point to a 'const char *' variable which
**    the user wants to hold the program invokation name
*/
extern int opt_parse (int argc, const void *argv, const struct opt_s *options,
                      const void *progname);
/*
** if using OPT_VERSION, call this to set the version to print
*/
extern void opt_setversion (const char *str);
/*
** print the help message
*/
extern void opt_usage (void);
/*
** set what to print after the program name in help message usage line
*/
extern void opt_setusage (const char *str);
/*
** for freestanding environment only:
** provide a print callback function for error messages
*/
extern void opt_setprintfn (void (*fn) (const char *, ...));
/*
** for hosted environment only:
** set whether to ignore case when checking for options
*/
extern void opt_caseless (int x);

#endif /* opt_h */
