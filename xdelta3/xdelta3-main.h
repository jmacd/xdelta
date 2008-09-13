/* xdelta 3 - delta compression tools and library
 * Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007,
 * Joshua P. MacDonald
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* This is all the extra stuff you need for convenience to users in a
 * command line application.  It contains these major components:
 *
 * 1. VCDIFF tools 2. external compression support (this is
 * POSIX-specific).  3. a general read/write loop that handles all of
 * the Xdelta decode/encode/VCDIFF-print functions 4. command-line
 * interpreter 5. an Xdelta application header which stores default
 * filename, external compression settings 6. output/error printing
 * 7. basic file support and OS interface
 */

/* TODO list: 1. do exact gzip-like filename, stdout handling.  make a
 * .vcdiff extension, refuse to encode to stdout without -cf, etc.
 * 2. Allow the user to add a comment string to the app header without
 * disturbing the default behavior.  3. "Source file must be seekable"
 * is not actually true for encoding, given current behavior.  Allow
 * non-seekable sources?  It would in theory let you use a fifo for
 * the source.
 */

/* On error handling and printing:
 *
 * The xdelta library sets stream->msg to indicate what condition
 * caused an internal failure, but many failures originate here and
 * are printed here.  The return convention is 0 for success, as
 * throughout Xdelta code, but special attention is required here for
 * the operating system calls with different error handling.  See the
 * main_file_* routines.  All errors in this file have a message
 * printed at the time of occurance.  Since some of these calls occur
 * within calls to the library, the error may end up being printed
 * again with a more general error message.
 */

/*********************************************************************/

#ifndef XD3_POSIX
#define XD3_POSIX 0
#endif
#ifndef XD3_STDIO
#define XD3_STDIO 0
#endif
#ifndef XD3_WIN32
#define XD3_WIN32 0
#endif
#ifndef NOT_MAIN
#define NOT_MAIN 0
#endif

/* Combines xd3_strerror() and strerror() */
const char* xd3_mainerror(int err_num);

/* XPRINTX (used by main) prefixes an "xdelta3: " to the output. */
#define XPR fprintf
#define NT stderr, "xdelta3: "

/* If none are set, default to posix. */
#if (XD3_POSIX + XD3_STDIO + XD3_WIN32) == 0
#undef XD3_POSIX
#define XD3_POSIX 1
#endif

/* Handle externally-compressed inputs. */
#ifndef EXTERNAL_COMPRESSION
#define EXTERNAL_COMPRESSION 1
#endif

#define PRINTHDR_SPECIAL -4378291

/* The number of soft-config variables.  */
#define XD3_SOFTCFG_VARCNT 7

/* this is used as in XPR(NT XD3_LIB_ERRMSG (stream, ret)) to print an
 * error message from the library. */
#define XD3_LIB_ERRMSG(stream, ret) "%s: %s\n", \
    xd3_errstring (stream), xd3_mainerror (ret)

#include <stdio.h>  /* fprintf */

#if XD3_POSIX
#include <unistd.h> /* close, read, write... */
#include <sys/types.h>
#include <fcntl.h>
#endif

#ifndef _WIN32
#include <unistd.h> /* lots */
#include <sys/time.h> /* gettimeofday() */
#include <sys/stat.h> /* stat() and fstat() */
#else
#if defined(_MSC_VER)
#define strtoll _strtoi64
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIFEXITED
#   define WIFEXITED(stat)  (((*((int *) &(stat))) & 0xff) == 0)
#endif
#ifndef WEXITSTATUS
#   define WEXITSTATUS(stat) (((*((int *) &(stat))) >> 8) & 0xff)
#endif
#ifndef S_ISREG
//#   ifdef S_IFREG
//#       define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
//#   else
#       define S_ISREG(m) 1
//#   endif
#endif /* !S_ISREG */

// For standard input/output handles
static STARTUPINFO winStartupInfo;
#endif

/**********************************************************************
 ENUMS and TYPES
 *********************************************************************/

/* These flags (mainly pertaining to main_read() operations) are set
 * in the main_file->flags variable.  All are related to with external
 * decompression support.
 *
 * RD_FIRST causes the external decompression check when the input is
 * first read.
 *
 * RD_NONEXTERNAL disables external decompression for reading a
 * compressed input, in the case of Xdelta inputs.  Note: Xdelta is
 * supported as an external compression type, which makes is the
 * reason for this flag.  An example to justify this is: to create a
 * delta between two files that are VCDIFF-compressed.  Two external
 * Xdelta decoders are run to supply decompressed source and target
 * inputs to the Xdelta encoder. */
typedef enum
{
  RD_FIRST        = (1 << 0),
  RD_NONEXTERNAL  = (1 << 1),
  RD_EXTERNAL_V1  = (1 << 2),
} xd3_read_flags;

/* main_file->mode values */
typedef enum
{
  XO_READ  = 0,
  XO_WRITE = 1,
} main_file_modes;

/* Main commands.  For example, CMD_PRINTHDR is the "xdelta printhdr"
 * command. */
typedef enum
{
  CMD_NONE = 0,
  CMD_PRINTHDR,
  CMD_PRINTHDRS,
  CMD_PRINTDELTA,
  CMD_RECODE,
  CMD_MERGE_ARG,
  CMD_MERGE,
#if XD3_ENCODER
  CMD_ENCODE,
#endif
  CMD_DECODE,
  CMD_TEST,
  CMD_CONFIG,
} xd3_cmd;

#if XD3_ENCODER
#define CMD_DEFAULT CMD_ENCODE
#define IS_ENCODE(cmd) (cmd == CMD_ENCODE)
#else
#define CMD_DEFAULT CMD_DECODE
#define IS_ENCODE(cmd) (0)
#endif

typedef struct _main_file        main_file;
typedef struct _main_extcomp     main_extcomp;
typedef struct _main_blklru      main_blklru;
typedef struct _main_blklru_list main_blklru_list;
typedef struct _main_merge       main_merge;
typedef struct _main_merge_list  main_merge_list;

/* The main_file object supports abstract system calls like open,
 * close, read, write, seek, stat.  The program uses these to
 * represent both seekable files and non-seekable files.  Source files
 * must be seekable, but the target input and any output file do not
 * require seekability.
 */
struct _main_file
{
#if XD3_STDIO
  FILE               *file;
#elif XD3_POSIX
  int                 file;
#elif XD3_WIN32
  HANDLE              file;
#endif

  int                 mode;          /* XO_READ and XO_WRITE */
  const char         *filename;      /* File name or /dev/stdin,
				      * /dev/stdout, /dev/stderr. */
  char               *filename_copy; /* File name or /dev/stdin,
				      * /dev/stdout, /dev/stderr. */
  const char         *realname;      /* File name or /dev/stdin,
				      * /dev/stdout, /dev/stderr. */
  const main_extcomp *compressor;    /* External compression struct. */
  int                 flags;         /* RD_FIRST, RD_NONEXTERNAL, ... */
  xoff_t              nread;         /* for input position */
  xoff_t              nwrite;        /* for output position */
  uint8_t            *snprintf_buf;  /* internal snprintf() use */
};

/* Various strings and magic values used to detect and call external
 * compression.  See below for examples. */
struct _main_extcomp
{
  const char    *recomp_cmdname;
  const char    *recomp_options;

  const char    *decomp_cmdname;
  const char    *decomp_options;

  const char    *ident;
  const char    *magic;
  usize_t        magic_size;
  int            flags;
};

/* This file implements a small LRU of source blocks.  For encoding purposes,
 * we prevent paging in blocks we've already scanned in the source (return
 * XD3_NOTAVAIL). */
struct _main_blklru_list
{
  main_blklru_list  *next;
  main_blklru_list  *prev;
};

struct _main_blklru
{
  uint8_t         *blk;
  xoff_t           blkno;
  main_blklru_list  link;
};

#define LRU_SIZE 32U
#define XD3_MINSRCWINSZ XD3_ALLOCSIZE

/* ... represented as a list (no cache index). */
XD3_MAKELIST(main_blklru_list,main_blklru,link);

/* Merge state: */

struct _main_merge_list
{
  main_merge_list  *next;
  main_merge_list  *prev;
};

struct _main_merge
{
  const char *filename;

  main_merge_list  link;
};

XD3_MAKELIST(main_merge_list,main_merge,link);

// TODO: really need to put options in a struct so that internal
// callers can easily reset state.

/* Program options: various command line flags and options. */
static int         option_stdout             = 0;
static int         option_force              = 0;
static int         option_verbose            = 0;
static int         option_quiet              = 0;
static int         option_use_appheader      = 1;
static uint8_t*    option_appheader          = NULL;
static int         option_use_secondary      = 0;
static char*       option_secondary          = NULL;
static int         option_use_checksum       = 1;
static int         option_use_altcodetable   = 0;
static char*       option_smatch_config      = NULL;
static int         option_no_compress        = 0;
static int         option_no_output          = 0; /* do not write output */
static const char *option_source_filename    = NULL;

static int         option_level              = XD3_DEFAULT_LEVEL;
static usize_t     option_iopt_size          = XD3_DEFAULT_IOPT_SIZE;
static usize_t     option_winsize            = XD3_DEFAULT_WINSIZE;
static usize_t     option_srcwinsz           = XD3_DEFAULT_SRCWINSZ;
static usize_t     option_sprevsz            = XD3_DEFAULT_SPREVSZ;

/* These variables are supressed to avoid their use w/o support.  main() warns
 * appropriately. */
#if EXTERNAL_COMPRESSION
static int         option_decompress_inputs  = 1;
static int         option_recompress_outputs = 1;
#endif

/* This is for comparing "printdelta" output without attention to
 * copy-instruction modes. */
#if VCDIFF_TOOLS
static int option_print_cpymode = 1; /* Note: see reset_defaults(). */ 
#endif

/* Static variables */
IF_DEBUG(static int main_mallocs = 0;)

static char*           program_name = NULL;
static uint8_t*        appheader_used = NULL;
static uint8_t*        main_bdata = NULL;
static usize_t         main_bsize = 0;

/* The LRU: obviously this is shared by all callers. */
static usize_t           lru_size = 0;
static main_blklru      *lru = NULL;  /* array of lru_size elts */
static main_blklru_list  lru_list;
static main_blklru_list  lru_free;
static int               do_not_lru = 0;  /* set to avoid lru */

static int lru_hits   = 0;
static int lru_misses = 0;
static int lru_filled = 0;

/* Hacks for VCDIFF tools */
static int allow_fake_source = 0;

/* recode_stream is used by both recode/merge for reading vcdiff inputs */
static xd3_stream *recode_stream = NULL;

/* merge_stream is used by merge commands for storing the source encoding */
static xd3_stream *merge_stream = NULL;

/* This array of compressor types is compiled even if EXTERNAL_COMPRESSION is
 * false just so the program knows the mapping of IDENT->NAME. */
static main_extcomp extcomp_types[] =
{
  /* The entry for xdelta3 must be 0 because the program_name is set there. */
  { "xdelta3",  "-cfq",  "xdelta3",    "-dcfq",  "X", "\xd6\xc3\xc4", 3,
    RD_NONEXTERNAL },
  { "bzip2",    "-cf",   "bzip2",      "-dcf",   "B", "BZh",          3, 0 },
  { "gzip",     "-cf",   "gzip",       "-dcf",   "G", "\037\213",     2, 0 },
  { "compress", "-cf",   "uncompress", "-cf",    "Z", "\037\235",     2, 0 },

  /* TODO: add commandline support for magic-less formats */
  /*{ "lzma", "-cf",   "lzma", "-dcf",   "M", "]\000", 2, 0 },*/
};

// };

static int main_input (xd3_cmd cmd, main_file *ifile,
                       main_file *ofile, main_file *sfile);
static void main_get_appheader (xd3_stream *stream, main_file *ifile,
				main_file *output, main_file *sfile);

static int main_help (void);

static int
main_version (void)
{
  /* $Format: "  DP(RINT \"Xdelta version $Xdelta3Version$, Copyright (C) 2007, 2008, Joshua MacDonald\n\");" $ */
  DP(RINT "Xdelta version 3.0u, Copyright (C) 2007, 2008, Joshua MacDonald\n");
  DP(RINT "Xdelta comes with ABSOLUTELY NO WARRANTY.\n");
  DP(RINT "This is free software, and you are welcome to redistribute it\n");
  DP(RINT "under certain conditions; see \"COPYING\" for details.\n");
  return EXIT_SUCCESS;
}

static int
main_config (void)
{
  main_version ();

  DP(RINT "EXTERNAL_COMPRESSION=%d\n", EXTERNAL_COMPRESSION);
  DP(RINT "GENERIC_ENCODE_TABLES=%d\n", GENERIC_ENCODE_TABLES);
  DP(RINT "GENERIC_ENCODE_TABLES_COMPUTE=%d\n", GENERIC_ENCODE_TABLES_COMPUTE);
  DP(RINT "REGRESSION_TEST=%d\n", REGRESSION_TEST);
  DP(RINT "SECONDARY_DJW=%d\n", SECONDARY_DJW);
  DP(RINT "SECONDARY_FGK=%d\n", SECONDARY_FGK);
  DP(RINT "UNALIGNED_OK=%d\n", UNALIGNED_OK);
  DP(RINT "VCDIFF_TOOLS=%d\n", VCDIFF_TOOLS);
  DP(RINT "XD3_ALLOCSIZE=%d\n", XD3_ALLOCSIZE);
  DP(RINT "XD3_DEBUG=%d\n", XD3_DEBUG);
  DP(RINT "XD3_ENCODER=%d\n", XD3_ENCODER);
  DP(RINT "XD3_POSIX=%d\n", XD3_POSIX);
  DP(RINT "XD3_STDIO=%d\n", XD3_STDIO);
  DP(RINT "XD3_WIN32=%d\n", XD3_WIN32);
  DP(RINT "XD3_USE_LARGEFILE64=%d\n", XD3_USE_LARGEFILE64);
  DP(RINT "XD3_DEFAULT_LEVEL=%d\n", XD3_DEFAULT_LEVEL);
  DP(RINT "XD3_DEFAULT_IOPT_SIZE=%d\n", XD3_DEFAULT_IOPT_SIZE);
  DP(RINT "XD3_DEFAULT_SPREVSZ=%d\n", XD3_DEFAULT_SPREVSZ);
  DP(RINT "XD3_DEFAULT_SRCWINSZ=%d\n", XD3_DEFAULT_SRCWINSZ);
  DP(RINT "XD3_DEFAULT_WINSIZE=%d\n", XD3_DEFAULT_WINSIZE);
  DP(RINT "XD3_HARDMAXWINSIZE=%d\n", XD3_HARDMAXWINSIZE);
  DP(RINT "sizeof(void*)=%ld\n", sizeof(void*));
  DP(RINT "sizeof(int)=%ld\n", sizeof(int));
  DP(RINT "sizeof(uint32_t)=%ld\n", sizeof(uint32_t));
  DP(RINT "sizeof(uint64_t)=%ld\n", sizeof(uint64_t));
  DP(RINT "sizeof(usize_t)=%ld\n", sizeof(usize_t));
  DP(RINT "sizeof(xoff_t)=%ld\n", sizeof(xoff_t));

  return EXIT_SUCCESS;
}

static void
reset_defaults(void)
{
  option_stdout = 0;
  option_force = 0;
  option_verbose = 0;
  option_quiet = 0;
  option_appheader = NULL;
  option_use_secondary = 0;
  option_secondary = NULL;
  option_use_altcodetable = 0;
  option_smatch_config = NULL;
  option_no_compress = 0;
  option_no_output = 0;
  option_source_filename = NULL;
  program_name = NULL;
  appheader_used = NULL;
  main_bdata = NULL;
  main_bsize = 0;
  lru_size = 0;
  lru = NULL;
  do_not_lru = 0;
  lru_hits   = 0;
  lru_misses = 0;
  lru_filled = 0;
  allow_fake_source = 0;
  option_smatch_config = NULL;

  option_use_appheader = 1;
  option_use_checksum = 1;
#if EXTERNAL_COMPRESSION
  option_decompress_inputs  = 1;
  option_recompress_outputs = 1;
#endif
#if VCDIFF_TOOLS
  option_print_cpymode = 1;
#endif
  option_level = XD3_DEFAULT_LEVEL;
  option_iopt_size = XD3_DEFAULT_IOPT_SIZE;
  option_winsize = XD3_DEFAULT_WINSIZE;
  option_srcwinsz = XD3_DEFAULT_SRCWINSZ;
  option_sprevsz = XD3_DEFAULT_SPREVSZ;
}

static void*
main_malloc1 (usize_t size)
{
  void* r = malloc (size);
  if (r == NULL) { XPR(NT "malloc: %s\n", xd3_mainerror (ENOMEM)); }
  else if (option_verbose > 3) { XPR(NT "malloc: %u: %p\n", size, r); }
  return r;
}

static void*
main_malloc (usize_t size)
{
  void *r = main_malloc1 (size);
  if (r) { IF_DEBUG (main_mallocs += 1); }
  return r;
}

static void*
main_alloc (void   *opaque,
	    usize_t  items,
	    usize_t  size)
{
  return main_malloc1 (items * size);
}

static void
main_free1 (void *opaque, void *ptr)
{
  if (option_verbose > 3) { XPR(NT "free: %p\n", ptr); }
  free (ptr);
}

static void
main_free (void *ptr)
{
  if (ptr)
    {
      IF_DEBUG (main_mallocs -= 1);
      main_free1 (NULL, ptr);
      IF_DEBUG (XD3_ASSERT(main_mallocs >= 0));
    }
}

/* This ensures that (ret = errno) always indicates failure, in case errno was
 * accidentally not set.  If this prints there's a bug somewhere. */
static int
get_errno (void)
{
#ifndef _WIN32
  if (errno == 0)
    {
      XPR(NT "you found a bug: expected errno != 0\n");
      errno = XD3_INTERNAL;
    }
  return errno;
#else
  DWORD errNum = GetLastError();
  if (errNum == NO_ERROR) {
	  errNum = XD3_INTERNAL;
  }
  return errNum;
#endif
}

const char*
xd3_mainerror(int err_num) {
#ifndef _WIN32
	const char* x = xd3_strerror (err_num);
	if (x != NULL) {
		return x;
	}
	return strerror(err_num);
#else
	static char err_buf[256];
	const char* x = xd3_strerror (err_num);
	if (x != NULL) {
		return x;
	}
	memset (err_buf, 0, 256);
	FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM |
		       FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err_num,
		MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		err_buf, 256, NULL);
	return err_buf;
#endif
}

static long
get_millisecs_now (void)
{
#ifndef _WIN32
  struct timeval tv;

  gettimeofday (& tv, NULL);

  return (tv.tv_sec) * 1000L + (tv.tv_usec) / 1000;
#else
  SYSTEMTIME st;
  FILETIME ft;
  __int64 *pi = (__int64*)&ft;
  GetLocalTime(&st);
  SystemTimeToFileTime(&st, &ft);
  return (long)((*pi) / 10000);
#endif
}

/* Always >= 1 millisec, right? */
static long
get_millisecs_since (void)
{
  static long last = 0;
  long now = get_millisecs_now();
  long diff = now - last;
  last = now;
  return diff;
}

static char*
main_format_bcnt (xoff_t r, char *buf)
{
  static const char* fmts[] = { "B", "KB", "MB", "GB" };
  usize_t i;

  for (i = 0; i < SIZEOF_ARRAY(fmts); i += 1)
    {
      if (r <= (10 * 1024) || i == (-1 + (int)SIZEOF_ARRAY(fmts)))
	{
	  sprintf (buf, "%"Q"u %s", r, fmts[i]);
	  break;
	}
      r /= 1024;
    }
  return buf;
}

static char*
main_format_rate (xoff_t bytes, long millis, char *buf)
{
  xoff_t r = (xoff_t)(1.0 * bytes / (1.0 * millis / 1000.0));
  static char lbuf[32];

  main_format_bcnt (r, lbuf);
  sprintf (buf, "%s/sec", lbuf);
  return buf;
}

static char*
main_format_millis (long millis, char *buf)
{
  if (millis < 1000)       { sprintf (buf, "%lu ms", millis); }
  else if (millis < 10000) { sprintf (buf, "%.1f sec", millis / 1000.0); }
  else                     { sprintf (buf, "%lu sec", millis / 1000L); }
  return buf;
}

/* A safe version of strtol for xoff_t. */
static int
main_strtoxoff (const char* s, xoff_t *xo, char which)
{
  char *e;
  xoff_t x;

  XD3_ASSERT(s && *s != 0);

  {
    /* Should check LONG_MIN, LONG_MAX, LLONG_MIN, LLONG_MAX? */
#if SIZEOF_XOFF_T == 4
    long xx = strtol (s, &e, 0);
#else
    long long xx = strtoll (s, &e, 0);
#endif

    if (xx < 0)
      {
	XPR(NT "-%c: negative integer: %s\n", which, s);
	return EXIT_FAILURE;
      }

    x = xx;
  }

  if (*e != 0)
    {
      XPR(NT "-%c: invalid integer: %s\n", which, s);
      return EXIT_FAILURE;
    }

  (*xo) = x;
  return 0;
}

static int
main_atou (const char* arg, usize_t *xo, usize_t low,
	   usize_t high, char which)
{
  xoff_t x;
  int ret;

  if ((ret = main_strtoxoff (arg, & x, which))) { return ret; }

  if (x < low)
    {
      XPR(NT "-%c: minimum value: %u\n", which, low);
      return EXIT_FAILURE;
    }
  if (high == 0)
    {
      high = USIZE_T_MAX;
    }
  if (x > high)
    {
      XPR(NT "-%c: maximum value: %u\n", which, high);
      return EXIT_FAILURE;
    }
  (*xo) = (usize_t)x;
  return 0;
}

/******************************************************************
 FILE BASICS
 ******************************************************************/

/* With all the variation in file system-call semantics, arguments,
 * return values and error-handling for the POSIX and STDIO file APIs,
 * the insides of these functions make me sick, which is why these
 * wrappers exist. */

#define XOPEN_OPNAME (xfile->mode == XO_READ ? "read" : "write")
#define XOPEN_STDIO  (xfile->mode == XO_READ ? "rb" : "wb")
#define XOPEN_POSIX  (xfile->mode == XO_READ ? \
		      O_RDONLY : O_WRONLY | O_CREAT | O_TRUNC)
#define XOPEN_MODE   (xfile->mode == XO_READ ? 0 : 0666)

#define XF_ERROR(op, name, ret) \
  do { if (!option_quiet) { XPR(NT "file %s failed: %s: %s: %s\n", (op), \
       XOPEN_OPNAME, (name), xd3_mainerror (ret)); } } while (0)

#if XD3_STDIO
#define XFNO(f) fileno(f->file)
#define XSTDOUT_XF(f) { (f)->file = stdout; (f)->filename = "/dev/stdout"; }
#define XSTDIN_XF(f)  { (f)->file = stdin;  (f)->filename = "/dev/stdin"; }

#elif XD3_POSIX
#define XFNO(f) f->file
#define XSTDOUT_XF(f) \
  { (f)->file = STDOUT_FILENO; (f)->filename = "/dev/stdout"; }
#define XSTDIN_XF(f) \
  { (f)->file = STDIN_FILENO;  (f)->filename = "/dev/stdin"; }

#elif XD3_WIN32
#define XFNO(f) -1
#define XSTDOUT_XF(f) { \
  (f)->file = GetStdHandle(STD_OUTPUT_HANDLE); \
  (f)->filename = "(stdout)"; \
  }
#define XSTDIN_XF(f) { \
  (f)->file = GetStdHandle(STD_INPUT_HANDLE); \
  (f)->filename = "(stdin)"; \
  }
#endif

static void
main_file_init (main_file *xfile)
{
  memset (xfile, 0, sizeof (*xfile));

#if XD3_POSIX
  xfile->file = -1;
#endif
#if XD3_WIN32
  xfile->file = INVALID_HANDLE_VALUE;
#endif
}

static int
main_file_isopen (main_file *xfile)
{
#if XD3_STDIO
  return xfile->file != NULL;

#elif XD3_POSIX
  return xfile->file != -1;

#elif XD3_WIN32
  return xfile->file != INVALID_HANDLE_VALUE;
#endif
}

static int
main_file_close (main_file *xfile)
{
  int ret = 0;

  if (! main_file_isopen (xfile))
    {
      return 0;
    }

#if XD3_STDIO
  ret = fclose (xfile->file);
  xfile->file = NULL;

#elif XD3_POSIX
  ret = close (xfile->file);
  xfile->file = -1;

#elif XD3_WIN32
  if (!CloseHandle(xfile->file)) {
    ret = get_errno ();
  }
  xfile->file = INVALID_HANDLE_VALUE;
#endif

  if (ret != 0) { XF_ERROR ("close", xfile->filename, ret = get_errno ()); }
  return ret;
}

static void
main_file_cleanup (main_file *xfile)
{
  XD3_ASSERT (xfile != NULL);

  if (main_file_isopen (xfile))
    {
      main_file_close (xfile);
    }

  if (xfile->snprintf_buf != NULL)
    {
      main_free(xfile->snprintf_buf);
      xfile->snprintf_buf = NULL;
    }

  if (xfile->filename_copy != NULL)
    {
      main_free(xfile->filename_copy);
      xfile->filename_copy = NULL;
    }
}

static int
main_file_open (main_file *xfile, const char* name, int mode)
{
  int ret = 0;

  xfile->mode = mode;

  XD3_ASSERT (name != NULL);
  XD3_ASSERT (! main_file_isopen (xfile));
  if (name[0] == 0)
    {
      XPR(NT "invalid file name: empty string\n");
      return XD3_INVALID;
    }

#if XD3_STDIO
  xfile->file = fopen (name, XOPEN_STDIO);

  ret = (xfile->file == NULL) ? get_errno () : 0;

#elif XD3_POSIX
  if ((ret = open (name, XOPEN_POSIX, XOPEN_MODE)) < 0)
    {
      ret = get_errno ();
    }
  else
    {
      xfile->file = ret;
      ret = 0;
    }

#elif XD3_WIN32
  xfile->file = CreateFile(name,
	  (mode == XO_READ) ? GENERIC_READ : GENERIC_WRITE,
	  FILE_SHARE_READ,
	  NULL,
	  (mode == XO_READ) ? OPEN_EXISTING :
			   (option_force ? CREATE_ALWAYS : CREATE_NEW),
	  FILE_ATTRIBUTE_NORMAL,
	  NULL);
  if (xfile->file == INVALID_HANDLE_VALUE) {
	  ret = get_errno ();
  }
#endif
  if (ret) { XF_ERROR ("open", name, ret); }
  else     { xfile->realname = name; xfile->nread = 0; }
  return ret;
}

static int
main_file_stat (main_file *xfile, xoff_t *size, int err_ifnoseek)
{
  int ret = 0;
#if XD3_WIN32
# if (_WIN32_WINNT >= 0x0500)
  LARGE_INTEGER li;
  if (GetFileSizeEx(xfile->file, &li) == 0)
    {
      ret = get_errno ();
    }
  else
    {
      *size = li.QuadPart;
    }
# else
  DWORD filesize = GetFileSize(xfile->file, NULL);
  if (filesize == INVALID_FILE_SIZE)
    {
      ret = GetLastError();
      if (ret != NO_ERROR)
	return ret;
    }
  *size = filesize;
# endif
#else
  struct stat sbuf;
  if (fstat (XFNO (xfile), & sbuf) < 0)
    {
      ret = get_errno ();
      if (err_ifnoseek)
	{
	  XF_ERROR ("stat", xfile->filename, ret);
	}
      return ret;
    }

  if (! S_ISREG (sbuf.st_mode))
    {
      if (err_ifnoseek)
	{
	  XPR(NT "source file must be seekable: %s\n", xfile->filename);
	}
      return ESPIPE;
    }
  (*size) = sbuf.st_size;
#endif
  return ret;
}

static int
main_file_exists (main_file *xfile)
{
  struct stat sbuf;
  return stat (xfile->filename, & sbuf) == 0 && S_ISREG (sbuf.st_mode);
}

#if (XD3_POSIX || EXTERNAL_COMPRESSION)
/* POSIX-generic code takes a function pointer to read() or write().
 * This calls the function repeatedly until the buffer is full or EOF.
 * The NREAD parameter is not set for write, NULL is passed.  Return
 * is signed, < 0 indicate errors, otherwise byte count. */
typedef int (xd3_posix_func) (int fd, uint8_t *buf, usize_t size);

static int
xd3_posix_io (int fd, uint8_t *buf, usize_t size,
	      xd3_posix_func *func, usize_t *nread)
{
  int ret;
  usize_t nproc = 0;

  while (nproc < size)
    {
      int result = (*func) (fd, buf + nproc, size - nproc);

      if (result < 0)
	{
	  ret = get_errno ();
	  if (ret != EAGAIN && ret != EINTR)
	    {
	      return ret;
	    }
	  result = 0;
	}

      if (nread != NULL && result == 0) { break; }

      nproc += result;
    }
  if (nread != NULL) { (*nread) = nproc; }
  return 0;
}
#endif

/* POSIX is unbuffered, while STDIO is buffered.  main_file_read()
 * should always be called on blocks. */
static int
main_file_read (main_file   *ifile,
	       uint8_t    *buf,
	       usize_t      size,
	       usize_t     *nread,
	       const char *msg)
{
  int ret = 0;

#if XD3_STDIO
  usize_t result;

  result = fread (buf, 1, size, ifile->file);

  if (result < size && ferror (ifile->file))
    {
      ret = get_errno ();
    }
  else
    {
      *nread = result;
    }

#elif XD3_POSIX
  ret = xd3_posix_io (ifile->file, buf, size, (xd3_posix_func*) &read, nread);

#elif XD3_WIN32
  DWORD nread2;
  if (ReadFile (ifile->file, buf, size, &nread2, NULL) == 0) {
	  ret = get_errno();
  } else {
      *nread = (usize_t)nread2;
  }
#endif

  if (ret)
    {
      XPR(NT "%s: %s: %s\n", msg, ifile->filename, xd3_mainerror (ret));
    }
  else
    {
      if (option_verbose > 3) { XPR(NT "main read: %s: %u\n",
				    ifile->filename, (*nread)); }
      ifile->nread += (*nread);
    }

  return ret;
}

static int
main_file_write (main_file *ofile, uint8_t *buf, usize_t size, const char *msg)
{
  int ret = 0;

#if XD3_STDIO
  usize_t result;

  result = fwrite (buf, 1, size, ofile->file);

  if (result != size) { ret = get_errno (); }

#elif XD3_POSIX
  ret = xd3_posix_io (ofile->file, buf, size, (xd3_posix_func*) &write, NULL);

#elif XD3_WIN32
  DWORD nwrite;
  if (WriteFile(ofile->file, buf, size, &nwrite, NULL) == 0) {
	  ret = get_errno ();
  } else {
	  if (size != nwrite) {
		  XPR(NT "Incorrect write count");
		  ret = XD3_INTERNAL;
	  }
  }
#endif

  if (ret)
    {
      XPR(NT "%s: %s: %s\n", msg, ofile->filename, xd3_mainerror (ret));
    }
  else
    {
      if (option_verbose > 3) { XPR(NT "main write: %s: %u\n",
				    ofile->filename, size); }
      ofile->nwrite += size;
    }

  return ret;
}

static int
main_file_seek (main_file *xfile, xoff_t pos)
{
  int ret = 0;

#if XD3_STDIO
  if (fseek (xfile->file, pos, SEEK_SET) != 0) { ret = get_errno (); }

#elif XD3_POSIX
  if ((xoff_t) lseek (xfile->file, pos, SEEK_SET) != pos)
    { ret = get_errno (); }

#elif XD3_WIN32
# if (_WIN32_WINNT >= 0x0500)
  LARGE_INTEGER move, out;
  move.QuadPart = pos;
  if (SetFilePointerEx(xfile->file, move, &out, FILE_BEGIN) == 0) {
	  ret = get_errno ();
  }
# else
  if (SetFilePointer(xfile->file, (LONG)pos, NULL, FILE_BEGIN) ==
					INVALID_SET_FILE_POINTER)
  {
	  ret = get_errno ();
  }
# endif
#endif

  if (ret)
    {
      XPR(NT "seek failed: %s: %s\n", xfile->filename, xd3_mainerror (ret));
    }

  return ret;
}

/* This function simply writes the stream output buffer, if there is
 * any, for encode, decode and recode commands.  (The VCDIFF tools use
 * main_print_func()). */
static int
main_write_output (xd3_stream* stream, main_file *ofile)
{
  int ret;

  if (option_no_output)
    {
      return 0;
    }

  if (stream->avail_out > 0 &&
      (ret = main_file_write (ofile, stream->next_out,
			      stream->avail_out, "write failed")))
    {
      return ret;
    }

  return 0;
}

static int
main_set_secondary_flags (xd3_config *config)
{
  int ret;
  if (option_use_secondary)
    {
      /* The default secondary compressor is DJW, if it's compiled. */
      if (option_secondary == NULL)
	{
	  if (SECONDARY_DJW)
	    {
	      config->flags |= XD3_SEC_DJW;
	    }
	}
      else
	{
	  if (strcmp (option_secondary, "fgk") == 0 && SECONDARY_FGK)
	    {
	      config->flags |= XD3_SEC_FGK;
	    }
	  else if (strncmp (option_secondary, "djw", 3) == 0 && SECONDARY_DJW)
	    {
	      usize_t level = XD3_DEFAULT_SECONDARY_LEVEL;

	      config->flags |= XD3_SEC_DJW;

	      if (strlen (option_secondary) > 3 &&
		  (ret = main_atou (option_secondary + 3,
				    &level,
				    0, 9, 'S')) != 0 &&
		  !option_quiet)
		{
		  return XD3_INVALID;
		}

	      /* XD3_SEC_NOXXXX flags disable secondary compression on
	       * a per-section basis.  For djw, ngroups=1 indicates
	       * minimum work, ngroups=0 uses default settings, which
	       * is > 1 groups by default. */
	      if (level < 1) { config->flags |= XD3_SEC_NODATA; }
	      if (level < 7) { config->sec_data.ngroups = 1; }
	      else { config->sec_data.ngroups = 0; }

	      if (level < 3) { config->flags |= XD3_SEC_NOINST; }
	      if (level < 8) { config->sec_inst.ngroups = 1; }
	      else { config->sec_inst.ngroups = 0; }

	      if (level < 5) { config->flags |= XD3_SEC_NOADDR; }
	      if (level < 9) { config->sec_addr.ngroups = 1; }
	      else { config->sec_addr.ngroups = 0; }
	    }
	  else if (strcmp (option_secondary, "none") == 0 && SECONDARY_DJW)
	    {
	      /* No secondary */
	    }
	  else
	    {
	      if (!option_quiet)
		{
		  XPR(NT "unrecognized secondary compressor type: %s\n",
		      option_secondary);
		  return XD3_INVALID;
		}
	    }
	}
    }

  return 0;
}

/******************************************************************
 VCDIFF TOOLS
 *****************************************************************/

#if VCDIFF_TOOLS
#include "xdelta3-merge.h"

#if defined(_WIN32) || defined(__DJGPP__)
/* According to the internet, Windows vsnprintf() differs from most
 * Unix implementations regarding the terminating 0 when the boundary
 * condition is met. It doesn't matter here, we don't rely on the
 * trailing 0.  Besides, both Windows and DJGPP vsnprintf return -1
 * upon truncation, which isn't C99 compliant. To overcome this,
 * recent MinGW runtimes provided their own vsnprintf (notice the
 * absence of the '_' prefix) but they were initially buggy.  So,
 * always use the native '_'-prefixed version with Win32. */
#include <stdarg.h>
#ifdef _WIN32
#define vsnprintf_func _vsnprintf
#else
#define vsnprintf_func  vsnprintf
#endif

int
snprintf_func (char *str, int n, char *fmt, ...)
{
  va_list a;
  int ret;
  va_start (a, fmt);
  ret = vsnprintf_func (str, n, fmt, a);
  va_end (a);
  if (ret < 0)
      ret = n;
  return ret;
}
#else
#define snprintf_func snprintf
#endif

/* The following macros let VCDIFF printing something printf-like with
 * main_file_write(), e.g.,:
 *
 *   VC(UT "trying to be portable: %d\n", x)VE;
 */
#define SNPRINTF_BUFSIZE 1024
#define VC do { if (((ret = snprintf_func
#define UT (char*)xfile->snprintf_buf, SNPRINTF_BUFSIZE,
#define VE ) >= SNPRINTF_BUFSIZE			       \
  && (ret = main_print_overflow(ret)) != 0)		       \
  || (ret = main_file_write(xfile, xfile->snprintf_buf,        \
			    ret, "print")) != 0)	       \
  { return ret; } } while (0)

static int
main_print_overflow (int x)
{
  XPR(NT "internal print buffer overflow: %d bytes\n", x);
  return XD3_INTERNAL;
}

/* This function prints a single VCDIFF window. */
static int
main_print_window (xd3_stream* stream, main_file *xfile)
{
  int ret;
  usize_t size = 0;

  VC(UT "  Offset Code Type1 Size1  @Addr1 + Type2 Size2 @Addr2\n")VE;

  while (stream->inst_sect.buf < stream->inst_sect.buf_max)
    {
      usize_t code = stream->inst_sect.buf[0];
      const uint8_t *addr_before = stream->addr_sect.buf;
      const uint8_t *inst_before = stream->inst_sect.buf;
      usize_t addr_bytes;
      usize_t inst_bytes;
      usize_t size_before = size;

      if ((ret = xd3_decode_instruction (stream)))
	{
	  XPR(NT "instruction decode error at %"Q"u: %s\n",
	      stream->dec_winstart + size, stream->msg);
	  return ret;
	}

      addr_bytes = stream->addr_sect.buf - addr_before;
      inst_bytes = stream->inst_sect.buf - inst_before;

      VC(UT "  %06"Q"u %03u  %s %6u", stream->dec_winstart + size, 
	 option_print_cpymode ? code : 0,
	 xd3_rtype_to_string ((xd3_rtype) stream->dec_current1.type, option_print_cpymode),
	 (usize_t) stream->dec_current1.size)VE;

      if (stream->dec_current1.type != XD3_NOOP)
	{
	  if (stream->dec_current1.type >= XD3_CPY)
	    {
	      if (stream->dec_current1.addr >= stream->dec_cpylen) 
		{
		  VC(UT " T@%-6u", 
		     stream->dec_current1.addr - stream->dec_cpylen)VE;
		} 
	      else
		{
		  VC(UT " S@%-6"Q"u", 
		     stream->dec_cpyoff + stream->dec_current1.addr)VE;
		}
	    }
	  else
	    {
	      VC(UT "        ")VE;
	    }

	  size += stream->dec_current1.size;
	}

      if (stream->dec_current2.type != XD3_NOOP)
	{
	  VC(UT "  %s %6u",
	     xd3_rtype_to_string ((xd3_rtype) stream->dec_current2.type,
				  option_print_cpymode),
	     (usize_t)stream->dec_current2.size)VE;

	  if (stream->dec_current2.type >= XD3_CPY)
	    {
	      if (stream->dec_current2.addr >= stream->dec_cpylen) 
		{
		  VC(UT " T@%-6u", 
		     stream->dec_current2.addr - stream->dec_cpylen)VE;
		} 
	      else
		{
		  VC(UT " S@%-6"Q"u", 
		     stream->dec_cpyoff + stream->dec_current2.addr)VE;
		}
	    }

	  size += stream->dec_current2.size;
	}

      VC(UT "\n")VE;

      if (option_verbose &&
	  addr_bytes + inst_bytes >= (size - size_before) &&
	  (stream->dec_current1.type >= XD3_CPY ||
	   stream->dec_current2.type >= XD3_CPY))
	{
	  VC(UT "  %06"Q"u (inefficiency) %u encoded as %u bytes\n",
	     stream->dec_winstart + size_before,
	     size - size_before,
	     addr_bytes + inst_bytes)VE;
	}
    }

  if (stream->dec_tgtlen != size && (stream->flags & XD3_SKIP_WINDOW) == 0)
    {
      XPR(NT "target window size inconsistency");
      return XD3_INTERNAL;
    }

  if (stream->dec_position != stream->dec_maxpos)
    {
      XPR(NT "target window position inconsistency");
      return XD3_INTERNAL;
    }

  if (stream->addr_sect.buf != stream->addr_sect.buf_max)
    {
      XPR(NT "address section inconsistency");
      return XD3_INTERNAL;
    }

  return 0;
}

static int
main_print_vcdiff_file (main_file *xfile, main_file *file, const char *type)
{
  int ret;  /* Used by above macros */
  if (file->filename)
    {
      VC(UT "XDELTA filename (%s):     %s\n", type,
	 file->filename)VE;
    }
  if (file->compressor)
    {
      VC(UT "XDELTA ext comp (%s):     %s\n", type,
	 file->compressor->recomp_cmdname)VE;
    }
  return 0;
}

/* This function prints a VCDIFF input, mainly for debugging purposes. */
static int
main_print_func (xd3_stream* stream, main_file *xfile)
{
  int ret;

  if (option_no_output)
    {
      return 0;
    }

  if (xfile->snprintf_buf == NULL)
    {
      if ((xfile->snprintf_buf = (uint8_t*)main_malloc(SNPRINTF_BUFSIZE)) == NULL)
	{
	  return ENOMEM;
	}
    }

  if (stream->dec_winstart == 0)
    {
      VC(UT "VCDIFF version:               0\n")VE;
      VC(UT "VCDIFF header size:           %d\n",
	 stream->dec_hdrsize)VE;
      VC(UT "VCDIFF header indicator:      ")VE;
      if ((stream->dec_hdr_ind & VCD_SECONDARY) != 0)
	VC(UT "VCD_SECONDARY ")VE;
      if ((stream->dec_hdr_ind & VCD_CODETABLE) != 0)
	VC(UT "VCD_CODETABLE ")VE;
      if ((stream->dec_hdr_ind & VCD_APPHEADER) != 0)
	VC(UT "VCD_APPHEADER ")VE;
      if (stream->dec_hdr_ind == 0)
	VC(UT "none")VE;
      VC(UT "\n")VE;

      IF_SEC(VC(UT "VCDIFF secondary compressor:  %s\n",
		stream->sec_type ? stream->sec_type->name : "none")VE);
      IF_NSEC(VC(UT "VCDIFF secondary compressor: unsupported\n")VE);

      if (stream->dec_hdr_ind & VCD_APPHEADER)
	{
	  uint8_t *apphead;
	  usize_t appheadsz;
	  ret = xd3_get_appheader (stream, & apphead, & appheadsz);

	  if (ret == 0 && appheadsz > 0)
	    {
	      int sq = option_quiet;
	      main_file i, o, s;
	      XD3_ASSERT (apphead != NULL);
	      VC(UT "VCDIFF application header:    ")VE;
	      if ((ret = main_file_write (xfile, apphead,
					  appheadsz, "print")) != 0)
		{ return ret; }
	      VC(UT "\n")VE;

	      main_file_init (& i);
	      main_file_init (& o);
	      main_file_init (& s);
	      option_quiet = 1;
	      main_get_appheader (stream, &i, & o, & s);
	      option_quiet = sq;
	      if ((ret = main_print_vcdiff_file (xfile, & o, "output")))
		{ return ret; }
	      if ((ret = main_print_vcdiff_file (xfile, & s, "source")))
		{ return ret; }
	      main_file_cleanup (& i);
	      main_file_cleanup (& o);
	      main_file_cleanup (& s);
	    }
	}
    }
  else
    {
      VC(UT "\n")VE;
    }

  VC(UT "VCDIFF window number:         %"Q"u\n", stream->current_window)VE;
  VC(UT "VCDIFF window indicator:      ")VE;
  if ((stream->dec_win_ind & VCD_SOURCE) != 0) VC(UT "VCD_SOURCE ")VE;
  if ((stream->dec_win_ind & VCD_TARGET) != 0) VC(UT "VCD_TARGET ")VE;
  if ((stream->dec_win_ind & VCD_ADLER32) != 0) VC(UT "VCD_ADLER32 ")VE;
  if (stream->dec_win_ind == 0) VC(UT "none")VE;
  VC(UT "\n")VE;

  if ((stream->dec_win_ind & VCD_ADLER32) != 0)
    {
      VC(UT "VCDIFF adler32 checksum:      %08X\n",
	 (usize_t)stream->dec_adler32)VE;
    }

  if (stream->dec_del_ind != 0)
    {
      VC(UT "VCDIFF delta indicator:       ")VE;
      if ((stream->dec_del_ind & VCD_DATACOMP) != 0) VC(UT "VCD_DATACOMP ")VE;
      if ((stream->dec_del_ind & VCD_INSTCOMP) != 0) VC(UT "VCD_INSTCOMP ")VE;
      if ((stream->dec_del_ind & VCD_ADDRCOMP) != 0) VC(UT "VCD_ADDRCOMP ")VE;
      if (stream->dec_del_ind == 0) VC(UT "none")VE;
      VC(UT "\n")VE;
    }

  if (stream->dec_winstart != 0)
    {
      VC(UT "VCDIFF window at offset:      %"Q"u\n", stream->dec_winstart)VE;
    }

  if (SRCORTGT (stream->dec_win_ind))
    {
      VC(UT "VCDIFF copy window length:    %u\n",
	 (usize_t)stream->dec_cpylen)VE;
      VC(UT "VCDIFF copy window offset:    %"Q"u\n",
	 stream->dec_cpyoff)VE;
    }

  VC(UT "VCDIFF delta encoding length: %u\n",
     (usize_t)stream->dec_enclen)VE;
  VC(UT "VCDIFF target window length:  %u\n",
     (usize_t)stream->dec_tgtlen)VE;

  VC(UT "VCDIFF data section length:   %u\n",
     (usize_t)stream->data_sect.size)VE;
  VC(UT "VCDIFF inst section length:   %u\n",
     (usize_t)stream->inst_sect.size)VE;
  VC(UT "VCDIFF addr section length:   %u\n",
     (usize_t)stream->addr_sect.size)VE;

  ret = 0;
  if ((stream->flags & XD3_JUST_HDR) != 0)
    {
      /* Print a header -- finished! */
      ret = PRINTHDR_SPECIAL;
    }
  else if ((stream->flags & XD3_SKIP_WINDOW) == 0)
    {
      ret = main_print_window (stream, xfile);
    }

  return ret;
}

static int
main_recode_copy (xd3_stream* stream,
		  xd3_output* output,
		  xd3_desect* input)
{
  int ret;

  XD3_ASSERT(output != NULL);
  XD3_ASSERT(output->next_page == NULL);

  if ((ret = xd3_decode_allocate (recode_stream,
				  input->size,
				  &output->base,
				  &output->avail)))
    {
      XPR(NT XD3_LIB_ERRMSG (stream, ret));
      return ret;
    }

  memcpy (output->base,
	  /* Note: decoder advances buf, so get base of buffer with
	   * buf_max - size */
	  input->buf_max - input->size,
	  input->size);
  output->next = input->size;
  return 0;
}

// Re-encode one window
static int
main_recode_func (xd3_stream* stream, main_file *ofile)
{
  int ret;
  xd3_source decode_source;

  XD3_ASSERT(stream->dec_state == DEC_FINISH);
  XD3_ASSERT(recode_stream->enc_state == ENC_INIT ||
	     recode_stream->enc_state == ENC_INPUT);

  // Copy partial decoder output to partial encoder inputs
  if ((ret = main_recode_copy (recode_stream,
			       DATA_HEAD(recode_stream),
			       &stream->data_sect)) ||
      (ret = main_recode_copy (recode_stream,
			       INST_HEAD(recode_stream),
			       &stream->inst_sect)) ||
      (ret = main_recode_copy (recode_stream,
			       ADDR_HEAD(recode_stream),
			       &stream->addr_sect)))
    {
      return ret;
    }

  // This jumps to xd3_emit_hdr()
  recode_stream->enc_state = ENC_FLUSH;
  recode_stream->avail_in = stream->dec_tgtlen;

  if (SRCORTGT (stream->dec_win_ind))
    {
      recode_stream->src = & decode_source;
      decode_source.srclen = stream->dec_cpylen;
      decode_source.srcbase = stream->dec_cpyoff;
    }

  if (option_use_checksum &&
      (stream->dec_win_ind & VCD_ADLER32) != 0)
    {
      recode_stream->flags |= XD3_ADLER32_RECODE;
      recode_stream->recode_adler32 = stream->dec_adler32;
    }

  if (option_use_appheader != 0 &&
      option_appheader != NULL)
    {
      xd3_set_appheader (recode_stream, option_appheader,
			 strlen ((char*) option_appheader));
    }
  else if (option_use_appheader != 0 &&
	   option_appheader == NULL)
    {
      if (stream->dec_appheader != NULL)
	{
	  xd3_set_appheader (recode_stream,
			     stream->dec_appheader, stream->dec_appheadsz);
	}
    }

  // Output loop
  for (;;)
    {
      switch((ret = xd3_encode_input (recode_stream)))
	{
	case XD3_INPUT: {
	  /* finished recoding one window */
	  stream->total_out = recode_stream->total_out;
	  return 0;
	}
	case XD3_OUTPUT: {
	  /* main_file_write below */
	  break;
	}
	case XD3_GOTHEADER:
	case XD3_WINSTART:
	case XD3_WINFINISH: {
	  /* ignore */
	  continue;
	}
	case XD3_GETSRCBLK:
	case 0: {
	    return XD3_INTERNAL;
	  }
	default:
	  return ret;
	}

      if ((ret = main_write_output (recode_stream, ofile)))
	{
	  return ret;
	}

      xd3_consume_output (recode_stream);
    }
}
#endif /* VCDIFF_TOOLS */

/*******************************************************************
 VCDIFF merging
 ******************************************************************/

#if VCDIFF_TOOLS
/* Modifies static state. */
static int
main_init_recode_stream (void)
{
  int ret;
  int stream_flags = XD3_ADLER32_NOVER | XD3_SKIP_EMIT;
  int recode_flags;
  xd3_config recode_config;

  XD3_ASSERT (recode_stream == NULL);

  if ((recode_stream = (xd3_stream*) main_malloc(sizeof(xd3_stream))) == NULL)
    {
      return ENOMEM;
    }

  recode_flags = (stream_flags & XD3_SEC_TYPE);

  recode_config.alloc = main_alloc;
  recode_config.freef = main_free1;

  xd3_init_config(&recode_config, recode_flags);

  if ((ret = main_set_secondary_flags (&recode_config)) ||
      (ret = xd3_config_stream (recode_stream, &recode_config)) ||
      (ret = xd3_encode_init_partial (recode_stream)) ||
      (ret = xd3_whole_state_init (recode_stream)))
    {
      XPR(NT XD3_LIB_ERRMSG (recode_stream, ret));
      xd3_free_stream (recode_stream);
      recode_stream = NULL;
      return ret;
    }

  return 0;
}

/* This processes the sequence of -m arguments.  The final input
 * is processed as part of the ordinary main_input() loop. */
static int
main_merge_arguments (main_merge_list* merges)
{
  int ret = 0;
  int count = 0;
  main_merge *merge = NULL;
  xd3_stream merge_input;

  if (main_merge_list_empty (merges))
    {
      return 0;
    }

  if ((ret = xd3_config_stream (& merge_input, NULL)) ||
      (ret = xd3_whole_state_init (& merge_input))) 
    {
      XPR(NT XD3_LIB_ERRMSG (& merge_input, ret));
      return ret;
    }

  merge = main_merge_list_front (merges);
  while (!main_merge_list_end (merges, merge))
    {
      main_file mfile;
      main_file_init (& mfile);
      mfile.filename = merge->filename;
      mfile.flags = RD_NONEXTERNAL;

      if ((ret = main_file_open (& mfile, merge->filename, XO_READ)))
        {
          goto error;
        }

      ret = main_input (CMD_MERGE_ARG, & mfile, NULL, NULL);

      if (ret == 0)
	{
	  if (count++ == 0)
	    {
	      /* The first merge source is the next merge input. */
	      xd3_swap_whole_state (& recode_stream->whole_target, 
				    & merge_input.whole_target);
	    }
	  else
	    {
	      /* Merge the recode_stream with merge_input. */
	      ret = xd3_merge_input_output (recode_stream,
					    & merge_input.whole_target);

	      /* Save the next merge source in merge_input. */
	      xd3_swap_whole_state (& recode_stream->whole_target,
				    & merge_input.whole_target);
	    }
	}

      main_file_cleanup (& mfile);

      if (recode_stream != NULL)
        {
          xd3_free_stream (recode_stream);
          main_free (recode_stream);
          recode_stream = NULL;
        }

      if (main_bdata != NULL)
        {
          main_free (main_bdata);
          main_bdata = NULL;
	  main_bsize = 0;
        }

      if (ret != 0)
        {
	  goto error;
        }

      merge = main_merge_list_next (merge);
    }

  XD3_ASSERT (merge_stream == NULL);

  if ((merge_stream = (xd3_stream*) main_malloc (sizeof(xd3_stream))) == NULL)
    {
      ret = ENOMEM;
      goto error;
    }

  if ((ret = xd3_config_stream (merge_stream, NULL)) ||
      (ret = xd3_whole_state_init (merge_stream)))
    {
      XPR(NT XD3_LIB_ERRMSG (& merge_input, ret));
      goto error;
    }

  xd3_swap_whole_state (& merge_stream->whole_target, 
			& merge_input.whole_target);
  ret = 0;
 error:
  xd3_free_stream (& merge_input);
  return ret;
}

/* This processes each window of the final merge input.  This routine
 * does not output, it buffers the entire delta into memory. */
static int
main_merge_func (xd3_stream* stream, main_file *no_write)
{
  int ret;

  if ((ret = xd3_whole_append_window (stream)))
    {
      return ret;
    }

  return 0;
}


/* This is called after all windows have been read, as a final step in
 * main_input().  This is only called for the final merge step. */
static int
main_merge_output (xd3_stream *stream, main_file *ofile)
{
  int ret;
  usize_t inst_pos = 0;
  xoff_t output_pos = 0;
  xd3_source recode_source;
  usize_t window_num = 0;
  int at_least_once = 0;

  /* merge_stream is set if there were arguments.  this stream's input
   * needs to be applied to the merge_stream source. */
  if ((merge_stream != NULL) &&
      (ret = xd3_merge_input_output (stream, 
				     & merge_stream->whole_target)))
    {
      XPR(NT XD3_LIB_ERRMSG (stream, ret));
      return ret;
    }

  if (option_use_appheader != 0 &&
      option_appheader != NULL)
    {
      xd3_set_appheader (recode_stream, option_appheader,
			 strlen ((char*) option_appheader));
    }

  /* Enter the ENC_INPUT state and bypass the next_in == NULL test
   * and (leftover) input buffering logic. */
  XD3_ASSERT(recode_stream->enc_state == ENC_INIT);
  recode_stream->enc_state = ENC_INPUT;
  recode_stream->next_in = main_bdata;
  recode_stream->flags |= XD3_FLUSH;

  /* This encodes the entire target. */
  while (inst_pos < stream->whole_target.instlen || !at_least_once)
    {
      xoff_t window_start = output_pos;
      int window_srcset = 0;
      xoff_t window_srcmin = 0;
      xoff_t window_srcmax = 0;
      usize_t window_pos = 0;
      usize_t window_size;

      /* at_least_once ensures that we encode at least one window,
       * which handles the 0-byte case. */
      at_least_once = 1;

      XD3_ASSERT (recode_stream->enc_state == ENC_INPUT);

      if ((ret = xd3_encode_input (recode_stream)) != XD3_WINSTART)
	{
	  XPR(NT "invalid merge state: %s\n", xd3_mainerror (ret));
	  return XD3_INVALID;
	}

      /* Window sizes must match from the input to the output, so that
       * target copies are in-range (and so that checksums carry
       * over). */
      XD3_ASSERT (window_num < stream->whole_target.wininfolen);
      window_size = stream->whole_target.wininfo[window_num].length;

      /* Output position should also match. */
      if (output_pos != stream->whole_target.wininfo[window_num].offset)
	{
	  XPR(NT "internal merge error: offset mismatch\n");
	  return XD3_INVALID;
	}

      if (option_use_checksum &&
	  (stream->dec_win_ind & VCD_ADLER32) != 0) 
	{
	  recode_stream->flags |= XD3_ADLER32_RECODE;
	  recode_stream->recode_adler32 = stream->whole_target.wininfo[window_num].adler32;
	}

      window_num++;

      if (main_bsize < window_size)
	{
	  main_free (main_bdata);
	  main_bdata = NULL;
	  main_bsize = 0;
	  if ((main_bdata = (uint8_t*) 
	       main_malloc (window_size)) == NULL)
	    {
	      return ENOMEM;
	    }
	  main_bsize = window_size;
	}

      /* This encodes a single target window. */
      while (window_pos < window_size &&
	     inst_pos < stream->whole_target.instlen)
	{
	  xd3_winst *inst = &stream->whole_target.inst[inst_pos];
	  usize_t take = min(inst->size, window_size - window_pos);
	  xoff_t addr;

	  switch (inst->type)
	    {
	    case XD3_RUN:
	      if ((ret = xd3_emit_run (recode_stream, window_pos, take,
				       stream->whole_target.adds[inst->addr])))
		{
		  return ret;
		}
	      break;

	    case XD3_ADD:
	      /* Adds are implicit, put them into the input buffer. */
	      memcpy (main_bdata + window_pos, 
		      stream->whole_target.adds + inst->addr, take);
	      break;

	    default: /* XD3_COPY + copy mode */
	      if (inst->mode != 0)
		{
		  if (window_srcset) {
		    window_srcmin = min(window_srcmin, inst->addr);
		    window_srcmax = max(window_srcmax, inst->addr + take);
		  } else {
		    window_srcset = 1;
		    window_srcmin = inst->addr;
		    window_srcmax = inst->addr + take;
		  }
		  addr = inst->addr;
		}
	      else 
		{
		  XD3_ASSERT (inst->addr >= window_start);
		  addr = inst->addr - window_start;
		}
	      IF_DEBUG1 (DP(RINT "[merge copy] winpos %u take %u addr %"Q"u mode %u\n",
			    window_pos, take, addr, inst->mode));
	      if ((ret = xd3_found_match (recode_stream, window_pos, take, 
					  addr, inst->mode != 0)))
		{
		  return ret;
		}
	      break;
	    }

	  window_pos += take;
	  output_pos += take;

	  if (take == inst->size)
	    {
	      inst_pos += 1;
	    }
	  else
	    {
	      /* Modify the instruction for the next pass. */
	      if (inst->type != XD3_RUN)
		{
		  inst->addr += take;
		}
	      inst->size -= take;
	    }
	}

      xd3_avail_input (recode_stream, main_bdata, window_pos);

      recode_stream->enc_state = ENC_INSTR;

      if (window_srcset) {
	recode_stream->srcwin_decided = 1;
	recode_stream->src = &recode_source;
	recode_source.srclen = window_srcmax - window_srcmin;
	recode_source.srcbase = window_srcmin;
	recode_stream->taroff = recode_source.srclen;
      } else {
	recode_stream->srcwin_decided = 0;
	recode_stream->src = NULL;
	recode_stream->taroff = 0;
      }

      for (;;)
	{
	  switch ((ret = xd3_encode_input (recode_stream)))
	    {
	    case XD3_INPUT: {
	      goto done_window;
	    }
	    case XD3_OUTPUT: {
	      /* main_file_write below */
	      break;
	    }
	    case XD3_GOTHEADER:
	    case XD3_WINSTART:
	    case XD3_WINFINISH: {
	      /* ignore */
	      continue;
	    }
	    case XD3_GETSRCBLK:
	    case 0: {
	      return XD3_INTERNAL;
	    }
	    default:
	      return ret;
	    }

	  if ((ret = main_write_output(recode_stream, ofile)))
	    {
	      return ret;
	    }

	  xd3_consume_output (recode_stream);
	}
    done_window:
      (void) 0;
    }

  return 0;
}
#endif

/*******************************************************************
 Input decompression, output recompression
 ******************************************************************/

#if EXTERNAL_COMPRESSION
/* This is tricky POSIX-specific code with lots of fork(), pipe(),
 * dup(), waitpid(), and exec() business.  Most of this code
 * originated in PRCS1, which did automatic package-file
 * decompression.  It works with both XD3_POSIX and XD3_STDIO file
 * disciplines.
 *
 * To automatically detect compressed inputs requires a child process
 * to reconstruct the input stream, which was advanced in order to
 * detect compression, because it may not be seekable.  In other
 * words, the main program reads part of the input stream, and if it
 * detects a compressed input it then forks a pipe copier process,
 * which copies the first-read block out of the main-program's memory,
 * then streams the remaining compressed input into the
 * input-decompression pipe.
 */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* Remember which pipe FD is which. */
#define PIPE_READ_FD  0
#define PIPE_WRITE_FD 1

static pid_t ext_subprocs[2];
static char* ext_tmpfile = NULL;

/* Like write(), but makes repeated calls to empty the buffer. */
static int
main_pipe_write (int outfd, uint8_t *exist_buf, usize_t remain)
{
  int ret;

  if ((ret = xd3_posix_io (outfd, exist_buf, remain,
			   (xd3_posix_func*) &write, NULL)))
    {
      XPR(NT "pipe write failed: %s", xd3_mainerror (ret));
      return ret;
    }

  return 0;
}

/* A simple error-reporting waitpid interface. */
static int
main_waitpid_check(pid_t pid)
{
  int status;
  int ret = 0;

  if (waitpid (pid, & status, 0) < 0)
    {
      ret = get_errno ();
      XPR(NT "compression subprocess: wait: %s\n", xd3_mainerror (ret));
    }
  else if (! WIFEXITED (status))
    {
      ret = ECHILD;
      XPR(NT "compression subprocess: signal %d\n",
	 WIFSIGNALED (status) ? WTERMSIG (status) : WSTOPSIG (status));
    }
  else if (WEXITSTATUS (status) != 0)
    {
      ret = ECHILD;
      XPR(NT "compression subprocess: exit %d\n", WEXITSTATUS (status));
    }

  return ret;
}

/* Wait for any existing child processes to check for abnormal exit. */
static int
main_external_compression_finish (void)
{
  int i;
  int ret;

  for (i = 0; i < 2; i += 1)
    {
      if (! ext_subprocs[i]) { continue; }

      if ((ret = main_waitpid_check (ext_subprocs[i])))
	{
	  return ret;
	}
    }

  return 0;
}

/* This runs as a forked process of main_input_decompress_setup() to
 * copy input to the decompression process.  First, the available
 * input is copied out of the existing buffer, then the buffer is
 * reused to continue reading from the compressed input file. */
static int
main_pipe_copier (uint8_t    *pipe_buf,
		  usize_t      pipe_bufsize,
		  usize_t      nread,
		  main_file   *ifile,
		  int         outfd)
{
  int ret;

  for (;;)
    {
      if (nread > 0 && (ret = main_pipe_write (outfd, pipe_buf, nread)))
	{
	  return ret;
	}

      if (nread < pipe_bufsize)
	{
	  break;
	}

      if ((ret = main_file_read (ifile, pipe_buf, pipe_bufsize,
				 & nread, "pipe read failed")) < 0)
	{
	  return ret;
	}
    }

  return 0;
}

/* This function is called after we have read some amount of data from
 * the input file and detected a compressed input.  Here we start a
 * decompression subprocess by forking twice.  The first process runs
 * the decompression command, the second process copies data to the
 * input of the first. */
static int
main_input_decompress_setup (const main_extcomp     *decomp,
			     main_file              *ifile,
			     uint8_t               *input_buf,
			     usize_t                 input_bufsize,
			     uint8_t               *pipe_buf,
			     usize_t                 pipe_bufsize,
			     usize_t                 pipe_avail,
			     usize_t                *nread)
{
  /* The two pipes: input and output file descriptors. */
  int outpipefd[2], inpipefd[2];
  int input_fd = -1;  /* The resulting input_fd (output of decompression). */
  pid_t decomp_id, copier_id;  /* The two subprocs. */
  int ret;

  outpipefd[0] = outpipefd[1] = -1;
  inpipefd[0]  = inpipefd[1]  = -1;

  if (pipe (outpipefd) || pipe (inpipefd))
    {
      XPR(NT "pipe failed: %s\n", xd3_mainerror (ret = get_errno ()));
      goto pipe_cleanup;
    }

  if ((decomp_id = fork ()) < 0)
    {
      XPR(NT "fork failed: %s\n", xd3_mainerror (ret = get_errno ()));
      goto pipe_cleanup;
    }

  /* The first child runs the decompression process: */
  if (decomp_id == 0)
    {
      /* Setup pipes: write to the outpipe, read from the inpipe. */
      if (dup2 (outpipefd[PIPE_WRITE_FD], STDOUT_FILENO) < 0 ||
	  dup2 (inpipefd[PIPE_READ_FD], STDIN_FILENO) < 0 ||
	  close (outpipefd[PIPE_READ_FD]) ||
	  close (outpipefd[PIPE_WRITE_FD]) ||
	  close (inpipefd[PIPE_READ_FD]) ||
	  close (inpipefd[PIPE_WRITE_FD]) ||
	  execlp (decomp->decomp_cmdname, decomp->decomp_cmdname,
		  decomp->decomp_options, NULL))
	{
	  XPR(NT "child process %s failed to execute: %s\n",
	      decomp->decomp_cmdname, xd3_mainerror (get_errno ()));
	}

      _exit (127);
    }

  ext_subprocs[0] = decomp_id;

  if ((copier_id = fork ()) < 0)
    {
      XPR(NT "fork failed: %s\n", xd3_mainerror (ret = get_errno ()));
      goto pipe_cleanup;
    }

  /* The second child runs the copier process: */
  if (copier_id == 0)
    {
      int exitval = 0;

      if (close (inpipefd[PIPE_READ_FD]) ||
	  main_pipe_copier (pipe_buf, pipe_bufsize, pipe_avail,
			    ifile, inpipefd[PIPE_WRITE_FD]) ||
	  close (inpipefd[PIPE_WRITE_FD]))
	{
	  XPR(NT "child copier process failed: %s\n",
	      xd3_mainerror (get_errno ()));
	  exitval = 1;
	}

      _exit (exitval);
    }

  ext_subprocs[1] = copier_id;

  /* The parent closes both pipes after duplicating the output of
   * compression. */
  input_fd = dup (outpipefd[PIPE_READ_FD]);

  if (input_fd < 0 ||
      main_file_close (ifile) ||
      close (outpipefd[PIPE_READ_FD]) ||
      close (outpipefd[PIPE_WRITE_FD]) ||
      close (inpipefd[PIPE_READ_FD]) ||
      close (inpipefd[PIPE_WRITE_FD]))
    {
      XPR(NT "dup/close failed: %s\n", xd3_mainerror (ret = get_errno ()));
      goto pipe_cleanup;
    }

#if XD3_STDIO
  /* Note: fdopen() acquires the fd, closes it when finished. */
  if ((ifile->file = fdopen (input_fd, "r")) == NULL)
    {
      XPR(NT "fdopen failed: %s\n", xd3_mainerror (ret = get_errno ()));
      goto pipe_cleanup;
    }

#elif XD3_POSIX
  ifile->file = input_fd;
#endif

  ifile->compressor = decomp;

  /* Now the input file is decompressed. */
  return main_file_read (ifile, input_buf, input_bufsize,
			 nread, "input decompression failed");

 pipe_cleanup:
  close (input_fd);
  close (outpipefd[PIPE_READ_FD]);
  close (outpipefd[PIPE_WRITE_FD]);
  close (inpipefd[PIPE_READ_FD]);
  close (inpipefd[PIPE_WRITE_FD]);
  return ret;
}


/* This routine is called when the first buffer of input data is read
 * by the main program (unless input decompression is disabled by
 * command-line option).  If it recognizes the magic number of a known
 * input type it invokes decompression.
 *
 * Skips decompression if the decompression type or the file type is
 * RD_NONEXTERNAL.
 *
 * Behaves exactly like main_file_read, otherwise.
 *
 * This function uses a separate buffer to read the first small block
 * of input.  If a compressed input is detected, the separate buffer
 * is passed to the pipe copier.  This avoids using the same size
 * buffer in both cases. */
static int
main_decompress_input_check (main_file   *ifile,
			    uint8_t    *input_buf,
			    usize_t      input_size,
			    usize_t     *nread)
{
  int ret;
  usize_t i;
  usize_t check_nread;
  uint8_t check_buf[XD3_ALLOCSIZE];

  if ((ret = main_file_read (ifile, check_buf,
			     min (input_size, XD3_ALLOCSIZE),
			     & check_nread, "input read failed")))
    {
      return ret;
    }

  for (i = 0; i < SIZEOF_ARRAY (extcomp_types); i += 1)
    {
      const main_extcomp *decomp = & extcomp_types[i];

      if ((check_nread > decomp->magic_size) &&
	  /* The following expr skips decompression if we are trying
	   * to read a VCDIFF input and that is the magic number. */
	  !((decomp->flags & RD_NONEXTERNAL) &&
	    (ifile->flags & RD_NONEXTERNAL)) &&
	  memcmp (check_buf, decomp->magic, decomp->magic_size) == 0)
	{
	  if (! option_quiet)
	    {
	      XPR(NT "%s | %s %s\n",
		 ifile->filename,
		 decomp->decomp_cmdname,
		 decomp->decomp_options);
	    }

	  return main_input_decompress_setup (decomp, ifile,
					      input_buf, input_size,
					      check_buf, XD3_ALLOCSIZE,
					      check_nread, nread);
	}
    }

  /* Now read the rest of the input block. */
  (*nread) = 0;

  if (check_nread == XD3_ALLOCSIZE)
    {
      ret = main_file_read (ifile, input_buf + XD3_ALLOCSIZE,
			    input_size - XD3_ALLOCSIZE, nread,
			    "input read failed");
    }

  memcpy (input_buf, check_buf, check_nread);

  (*nread) += check_nread;

  return 0;
}

/* This is called when the source file needs to be decompressed.  We
 * fork/exec a decompression command with the proper input and output
 * to a temporary file. */
static int
main_decompress_source (main_file *sfile, xd3_source *source)
{
  const main_extcomp *decomp = sfile->compressor;
  pid_t decomp_id;  /* One subproc. */
  int   input_fd  = -1;
  int   output_fd = -1;
  int   ret;
  char *tmpname = NULL;
  char *tmpdir  = getenv ("TMPDIR");
  static const char tmpl[] = "/xd3src.XXXXXX";

  /* Make a template for mkstmp() */
  if (tmpdir == NULL) { tmpdir = "/tmp"; }
  if ((tmpname =
       (char*) main_malloc (strlen (tmpdir) + sizeof (tmpl) + 1)) == NULL)
    {
      return ENOMEM;
    }
  sprintf (tmpname, "%s%s", tmpdir, tmpl);

  XD3_ASSERT (ext_tmpfile == NULL);
  ext_tmpfile = tmpname;

  /* Open the output FD. */
  if ((output_fd = mkstemp (tmpname)) < 0)
    {
      XPR(NT "mkstemp failed: %s: %s",
	  tmpname, xd3_mainerror (ret = get_errno ()));
      goto cleanup;
    }

  /* Copy the input FD, reset file position. */
  XD3_ASSERT (main_file_isopen (sfile));
#if XD3_STDIO
  if ((input_fd = dup (fileno (sfile->file))) < 0)
    {
      XPR(NT "dup failed: %s", xd3_mainerror (ret = get_errno ()));
      goto cleanup;
    }
  main_file_close (sfile);
  sfile->file = NULL;
#elif XD3_POSIX
  input_fd = sfile->file;
  sfile->file = -1;
#endif

  if ((ret = lseek (input_fd, SEEK_SET, 0)) != 0)
    {
      XPR(NT "lseek failed: : %s", xd3_mainerror (ret = get_errno ()));
      goto cleanup;
    }

  if ((decomp_id = fork ()) < 0)
    {
      XPR(NT "fork failed: %s", xd3_mainerror (ret = get_errno ()));
      goto cleanup;
    }

  /* The child runs the decompression process: */
  if (decomp_id == 0)
    {
      /* Setup pipes: write to the output file, read from the pipe. */
      if (dup2 (input_fd, STDIN_FILENO) < 0 ||
	  dup2 (output_fd, STDOUT_FILENO) < 0 ||
	  execlp (decomp->decomp_cmdname, decomp->decomp_cmdname,
		  decomp->decomp_options, NULL))
	{
	  XPR(NT "child process %s failed to execute: %s\n",
		   decomp->decomp_cmdname, xd3_mainerror (get_errno ()));
	}

      _exit (127);
    }

  close (input_fd);
  close (output_fd);
  input_fd  = -1;
  output_fd = -1;

  /* Then wait for completion. */
  if ((ret = main_waitpid_check (decomp_id)))
    {
      goto cleanup;
    }

  /* Open/stat the decompressed source file. */
  if ((ret = main_file_open (sfile, tmpname, XO_READ))) { goto cleanup; }
  if ((ret = main_file_stat (sfile, & source->size, 1))) { goto cleanup; }
  return 0;

 cleanup:
  close (input_fd);
  close (output_fd);
  if (tmpname) { free (tmpname); }
  ext_tmpfile = NULL;
  return ret;
}

/* Initiate re-compression of the output stream.  This is easier than
 * input decompression because we know beforehand that the stream will
 * be compressed, whereas the input has already been read when we
 * decide it should be decompressed.  Thus, it only requires one
 * subprocess and one pipe. */
static int
main_recompress_output (main_file *ofile)
{
  pid_t recomp_id;  /* One subproc. */
  int   pipefd[2];  /* One pipe. */
  int   output_fd = -1;
  int   ret;
  const main_extcomp *recomp = ofile->compressor;

  pipefd[0] = pipefd[1] = -1;

  if (pipe (pipefd))
    {
      XPR(NT "pipe failed: %s\n", xd3_mainerror (ret = get_errno ()));
      goto pipe_cleanup;
    }

  if ((recomp_id = fork ()) < 0)
    {
      XPR(NT "fork failed: %s\n", xd3_mainerror (ret = get_errno ()));
      goto pipe_cleanup;
    }

  /* The child runs the recompression process: */
  if (recomp_id == 0)
    {
      /* Setup pipes: write to the output file, read from the pipe. */
      if (dup2 (XFNO (ofile), STDOUT_FILENO) < 0 ||
	  dup2 (pipefd[PIPE_READ_FD], STDIN_FILENO) < 0 ||
	  close (pipefd[PIPE_READ_FD]) ||
	  close (pipefd[PIPE_WRITE_FD]) ||
	  execlp (recomp->recomp_cmdname, recomp->recomp_cmdname,
		  recomp->recomp_options, NULL))
	{
	  XPR(NT "child process %s failed to execute: %s\n",
	      recomp->recomp_cmdname, xd3_mainerror (get_errno ()));
	}

      _exit (127);
    }

  ext_subprocs[0] = recomp_id;

  /* The parent closes both pipes after duplicating the output-fd for
   * writing to the compression pipe. */
  output_fd = dup (pipefd[PIPE_WRITE_FD]);

  if (output_fd < 0 ||
      main_file_close (ofile) ||
      close (pipefd[PIPE_READ_FD]) ||
      close (pipefd[PIPE_WRITE_FD]))
    {
      XPR(NT "close failed: %s\n", xd3_mainerror (ret = get_errno ()));
      goto pipe_cleanup;
    }

#if XD3_STDIO
  /* Note: fdopen() acquires the fd, closes it when finished. */
  if ((ofile->file = fdopen (output_fd, "w")) == NULL)
    {
      XPR(NT "fdopen failed: %s\n", xd3_mainerror (ret = get_errno ()));
      goto pipe_cleanup;
    }

#elif XD3_POSIX
  ofile->file = output_fd;
#endif

  /* Now the output file will be compressed. */
  return 0;

 pipe_cleanup:
  close (output_fd);
  close (pipefd[PIPE_READ_FD]);
  close (pipefd[PIPE_WRITE_FD]);
  return ret;
}
#endif /* EXTERNAL_COMPRESSION */

/* Identify the compressor that was used based on its ident string,
 * which is passed in the application header. */
static const main_extcomp*
main_ident_compressor (const char *ident)
{
  usize_t i;

  for (i = 0; i < SIZEOF_ARRAY (extcomp_types); i += 1)
    {
      if (strcmp (extcomp_types[i].ident, ident) == 0)
	{
	  return & extcomp_types[i];
	}
    }

  return NULL;
}

/* Return the main_extcomp record to use for this identifier, if possible. */
static const main_extcomp*
main_get_compressor (const char *ident)
{
  const main_extcomp *ext = main_ident_compressor (ident);

  if (ext == NULL)
    {
      if (! option_quiet)
	{
	  XPR(NT "warning: cannot recompress output: "
		   "unrecognized external compression ID: %s\n", ident);
	}
      return NULL;
    }
  else if (! EXTERNAL_COMPRESSION)
    {
      if (! option_quiet)
	{
	  XPR(NT "warning: external support not compiled: "
		   "original input was compressed: %s\n", ext->recomp_cmdname);
	}
      return NULL;
    }
  else
    {
      return ext;
    }
}

/*********************************************************************
 APPLICATION HEADER
 *******************************************************************/

#if XD3_ENCODER
static const char*
main_apphead_string (const char* x)
{
  const char *y;

  if (x == NULL) { return ""; }

  if (strcmp (x, "/dev/stdin") == 0 ||
      strcmp (x, "/dev/stdout") == 0 ||
      strcmp (x, "/dev/stderr") == 0) { return "-"; }

  // TODO: this is not portable
  return (y = strrchr (x, '/')) == NULL ? x : y + 1;
}

static int
main_set_appheader (xd3_stream *stream, main_file *input, main_file *sfile)
{
  /* The user may disable the application header.  Once the appheader
   * is set, this disables setting it again. */
  if (appheader_used || ! option_use_appheader) { return 0; }

  /* The user may specify the application header, otherwise format the
     default header. */
  if (option_appheader)
    {
      appheader_used = option_appheader;
    }
  else
    {
      const char *iname;
      const char *icomp;
      const char *sname;
      const char *scomp;
      int len;

      iname = main_apphead_string (input->filename);
      icomp = (input->compressor == NULL) ? "" : input->compressor->ident;
      len = strlen (iname) + strlen (icomp) + 2;

      if (sfile->filename != NULL)
	{
	  sname = main_apphead_string (sfile->filename);
	  scomp = (sfile->compressor == NULL) ? "" : sfile->compressor->ident;
	  len += strlen (sname) + strlen (scomp) + 2;
	}
      else
	{
	  sname = scomp = "";
	}

      if ((appheader_used = (uint8_t*) main_malloc (len)) == NULL)
	{
	  return ENOMEM;
	}

      if (sfile->filename == NULL)
	{
	  sprintf ((char*)appheader_used, "%s/%s", iname, icomp);
	}
      else
	{
	  sprintf ((char*)appheader_used, "%s/%s/%s/%s",
		   iname, icomp, sname, scomp);
	}
    }

  xd3_set_appheader (stream, appheader_used, strlen ((char*)appheader_used));

  return 0;
}
#endif

static void
main_get_appheader_params (main_file *file, char **parsed,
			   int output, const char *type,
			   main_file *other)
{
  /* Set the filename if it was not specified.  If output, option_stdout (-c)
   * overrides. */
  if (file->filename == NULL &&
      ! (output && option_stdout) &&
      strcmp (parsed[0], "-") != 0)
    {
      file->filename = parsed[0];

      if (other->filename != NULL) {
	/* Take directory from the other file, if it has one. */
	/* TODO: This results in nonsense names like /dev/foo.tar.gz
	 * and probably the filename-default logic interferes with
	 * multi-file operation and the standard file extension?
	 * Possibly the name header is bad, should be off by default.
	 * Possibly we just want to remember external/compression
	 * settings. */
	char *last_slash = strrchr(other->filename, '/');

	if (last_slash != NULL) {
	  int dlen = last_slash - other->filename;

	  XD3_ASSERT(file->filename_copy == NULL);
	  file->filename_copy =
	    (char*) main_malloc(dlen + 2 + strlen(file->filename));

	  strncpy(file->filename_copy, other->filename, dlen);
	  file->filename_copy[dlen] = '/';
	  strcpy(file->filename_copy + dlen + 1, parsed[0]);

	  file->filename = file->filename_copy;
	}
      }

      if (! option_quiet)
	{
	  XPR(NT "using default %s filename: %s\n", type, file->filename);
	}
    }

  /* Set the compressor, initiate de/recompression later. */
  if (file->compressor == NULL && *parsed[1] != 0)
    {
      file->compressor = main_get_compressor (parsed[1]);
    }
}

static void
main_get_appheader (xd3_stream *stream, main_file *ifile,
		    main_file *output, main_file *sfile)
{
  uint8_t *apphead;
  usize_t appheadsz;
  int ret;

  /* The user may disable the application header.  Once the appheader
   * is set, this disables setting it again. */
  if (! option_use_appheader) { return; }

  ret = xd3_get_appheader (stream, & apphead, & appheadsz);

  /* Ignore failure, it only means we haven't received a header yet. */
  if (ret != 0) { return; }

  if (appheadsz > 0)
    {
      char *start = (char*)apphead;
      char *slash;
      int   place = 0;
      char *parsed[4];

      memset (parsed, 0, sizeof (parsed));

      while ((slash = strchr (start, '/')) != NULL)
	{
	  *slash = 0;
	  parsed[place++] = start;
	  start = slash + 1;
	}

      parsed[place++] = start;

      /* First take the output parameters. */
      if (place == 2 || place == 4)
	{
	  main_get_appheader_params (output, parsed, 1, "output", ifile);
	}

      /* Then take the source parameters. */
      if (place == 4)
	{
	  main_get_appheader_params (sfile, parsed+2, 0, "source", ifile);
	}
    }

  option_use_appheader = 0;
  return;
}

/*********************************************************************
 Main I/O routines
 **********************************************************************/

/* This function acts like the above except it may also try to
 * recognize a compressed input when the first buffer of data is read.
 * The EXTERNAL_COMPRESSION code is called to search for magic
 * numbers. */
static int
main_read_primary_input (main_file   *ifile,
			 uint8_t    *buf,
			 usize_t      size,
			 usize_t     *nread)
{
#if EXTERNAL_COMPRESSION
  if (option_decompress_inputs && ifile->flags & RD_FIRST)
    {
      ifile->flags &= ~RD_FIRST;

      return main_decompress_input_check (ifile, buf, size, nread);
    }
#endif

  return main_file_read (ifile, buf, size, nread, "input read failed");
}

/* Open the main output file, sets a default file name, initiate
 * recompression.  This function is expected to fprint any error
 * messages. */
static int
main_open_output (xd3_stream *stream, main_file *ofile)
{
  int ret;

  if (option_no_output)
    {
      return 0;
    }

  if (ofile->filename == NULL)
    {
      XSTDOUT_XF (ofile);

      if (option_verbose > 1)
	{
	  XPR(NT "using standard output: %s\n", ofile->filename);
	}
    }
  else
    {
      /* Stat the file to check for overwrite. */
      if (option_force == 0 && main_file_exists (ofile))
	{
	  if (!option_quiet)
	    {
	      XPR(NT "to overwrite output file specify -f: %s\n",
		  ofile->filename);
	    }
	  return EEXIST;
	}

      if ((ret = main_file_open (ofile, ofile->filename, XO_WRITE)))
	{
	  return ret;
	}

      if (option_verbose > 1) { XPR(NT "output file: %s\n", ofile->filename); }
    }

#if EXTERNAL_COMPRESSION
  /* Do output recompression. */
  if (ofile->compressor != NULL && option_recompress_outputs == 1)
    {
      if (! option_quiet)
	{
	  XPR(NT "%s %s | %s\n",
	     ofile->compressor->recomp_cmdname,
	     ofile->compressor->recomp_options,
	     ofile->filename);
	}

      if ((ret = main_recompress_output (ofile)))
	{
	  return ret;
	}
    }
#endif

  return 0;
}

/* This is called at different times for encoding and decoding.  The
 * encoder calls it immediately, the decoder delays until the
 * application header is received.  Stream may be NULL, in which case
 * xd3_set_source is not called. */
static int
main_set_source (xd3_stream *stream, int cmd,
		 main_file *sfile, xd3_source *source)
{
  int ret = 0;
  usize_t i;
  uint8_t *tmp_buf = NULL;

  /* Open it, check for seekability, set required xd3_source fields. */
  if (allow_fake_source)
    {
      sfile->mode = XO_READ;
      sfile->realname = sfile->filename;
      sfile->nread = 0;
      source->size = XOFF_T_MAX;
    }
  else
    {
      if ((ret = main_file_open (sfile, sfile->filename, XO_READ)) ||
	  (ret = main_file_stat (sfile, & source->size, 1)))
	{
	  goto error;
	}
    }

  source->name     = sfile->filename;
  source->ioh      = sfile;
  source->curblkno = (xoff_t) -1;
  source->curblk   = NULL;

#if EXTERNAL_COMPRESSION
  if (option_decompress_inputs)
    {
      /* If encoding, read the header to check for decompression. */
      if (IS_ENCODE (cmd))
	{
	  usize_t nread;
	  tmp_buf = (uint8_t*) main_malloc (XD3_ALLOCSIZE);

	  if ((ret = main_file_read (sfile, tmp_buf, XD3_ALLOCSIZE,
				     & nread, "source read failed")))
	    {
	      goto error;
	    }

	  /* Check known magic numbers. */
	  for (i = 0; i < SIZEOF_ARRAY (extcomp_types); i += 1)
	    {
	      const main_extcomp *decomp = & extcomp_types[i];

	      if ((nread > decomp->magic_size) &&
		  memcmp (tmp_buf, decomp->magic, decomp->magic_size) == 0)
		{
		  sfile->compressor = decomp;
		  break;
		}
	    }

	  if (sfile->compressor == NULL)
	    {
	      if (option_verbose > 2)
		{
		  XPR(NT "source block 0 read (not compressed)\n");
		}
	    }
	}

      /* In either the encoder or decoder, start decompression. */
      if (sfile->compressor)
	{
	  xoff_t osize = source->size;

	  if ((ret = main_decompress_source (sfile, source)))
	    {
	      goto error;
	    }

	  if (! option_quiet)
	    {
	      char s1[32], s2[32];
	      XPR(NT "%s | %s %s => %s %.1f%% [ %s , %s ]\n",
		 sfile->filename,
		 sfile->compressor->decomp_cmdname,
		 sfile->compressor->decomp_options,
		 sfile->realname,
		 100.0 * source->size / osize,
		 main_format_bcnt (osize, s1),
		 main_format_bcnt (source->size, s2));
	    }
	}
    }
#endif

  /* At this point we know source->size.
   * Source buffer, blksize, LRU init. */
  if (source->size < option_srcwinsz)
    {
      /* Reduce sizes to actual source size, read whole file */
      option_srcwinsz = source->size;
      source->blksize = source->size;
      lru_size = 1;
    }
  else
    {
      option_srcwinsz = max(option_srcwinsz, XD3_MINSRCWINSZ);

      source->blksize = (option_srcwinsz / LRU_SIZE);
      lru_size = LRU_SIZE;
    }

  main_blklru_list_init (& lru_list);
  main_blklru_list_init (& lru_free);

  if (option_verbose)
    {
      static char buf[32];

      XPR(NT "source %s winsize %s size %"Q"u\n",
	  sfile->filename, main_format_bcnt(option_srcwinsz, buf),
	  source->size);
    }

  if (option_verbose > 1)
    {
      XPR(NT "source block size: %u\n", source->blksize);
    }

  if ((lru = (main_blklru*)
       main_malloc (sizeof (main_blklru) * lru_size)) == NULL)
    {
      ret = ENOMEM;
      goto error;
    }

  for (i = 0; i < lru_size; i += 1)
    {
      lru[i].blkno = (xoff_t) -1;

      if ((lru[i].blk = (uint8_t*) main_malloc (source->blksize)) == NULL)
	{
	  ret = ENOMEM;
	  goto error;
	}

      main_blklru_list_push_back (& lru_free, & lru[i]);
    }

  if (stream && (ret = xd3_set_source (stream, source)))
    {
      XPR(NT XD3_LIB_ERRMSG (stream, ret));
      goto error;
    }

 error:
  if (tmp_buf != NULL)
    {
      main_free (tmp_buf);
    }

  return ret;
}

static usize_t
main_get_winsize (main_file *ifile) {
  xoff_t file_size;
  usize_t size = option_winsize;

  if (main_file_stat (ifile, &file_size, 0) == 0)
    {
      size = (usize_t) min(file_size, (xoff_t) size);
    }

  size = max(size, XD3_ALLOCSIZE);

  if (option_verbose > 1)
    {
      XPR(NT "input window size: %u\n", size);
    }

  return size;
}

/*******************************************************************
 Source routines
 *******************************************************************/

/* This is the callback for reading a block of source.  This function
 * is blocking and it implements a small LRU.
 *
 * Note that it is possible for main_input() to handle getblk requests
 * in a non-blocking manner.  If the callback is NULL then the caller
 * of xd3_*_input() must handle the XD3_GETSRCBLK return value and
 * fill the source in the same way.  See xd3_getblk for details.  To
 * see an example of non-blocking getblk, see xdelta-test.h. */
static int
main_getblk_func (xd3_stream *stream,
		  xd3_source *source,
		  xoff_t      blkno)
{
  int ret;
  xoff_t pos = blkno * source->blksize;
  main_file *sfile = (main_file*) source->ioh;
  main_blklru *blru  = NULL;
  usize_t onblk = xd3_bytes_on_srcblk_fast (source, blkno);
  usize_t nread;
  usize_t i;

  if (allow_fake_source)
    {
      source->curblkno = blkno;
      source->onblk    = onblk;
      source->curblk   = lru[0].blk;
      return 0;
    }

  if (do_not_lru)
    {
      /* Direct lookup assumes sequential scan w/o skipping blocks. */
      int idx = blkno % lru_size;
      if (lru[idx].blkno == blkno)
	{
	  source->curblkno = blkno;
	  source->onblk    = onblk;
	  source->curblk   = lru[idx].blk;
	  lru_hits += 1;
	  return 0;
	}

      if (lru[idx].blkno != (xoff_t)-1 &&
	  lru[idx].blkno != (xoff_t)(blkno - lru_size))
	{
	  return XD3_TOOFARBACK;
	}
    }
  else
    {
      /* Sequential search through LRU. */
      for (i = 0; i < lru_size; i += 1)
	{
	  if (lru[i].blkno == blkno)
	    {
	      main_blklru_list_remove (& lru[i]);
	      main_blklru_list_push_back (& lru_list, & lru[i]);

	      source->curblkno = blkno;
	      source->onblk    = onblk;
	      source->curblk   = lru[i].blk;
	      lru_hits += 1;
	      return 0;
	    }
	}
    }

  if (! main_blklru_list_empty (& lru_free))
    {
      blru = main_blklru_list_pop_front (& lru_free);
    }
  else if (! main_blklru_list_empty (& lru_list))
    {
      if (do_not_lru) {
	blru = & lru[blkno % lru_size];
	main_blklru_list_remove(blru);
      } else {
	blru = main_blklru_list_pop_front (& lru_list);
      }
      lru_misses += 1;
    }

  lru_filled += 1;

  if ((ret = main_file_seek (sfile, pos)))
    {
      return ret;
    }

  if ((ret = main_file_read (sfile, (uint8_t*) blru->blk, source->blksize,
			     & nread, "source read failed")))
    {
      return ret;
    }

  if (nread != onblk)
    {
      XPR(NT "source file size change: %s\n", sfile->filename);
      return XD3_INTERNAL;
    }

  main_blklru_list_push_back (& lru_list, blru);

  if (option_verbose > 3)
    {
      if (blru->blkno != (xoff_t)-1)
	{
	  XPR(NT "source block %"Q"u ejects %"Q"u (lru_hits=%u, "
	      "lru_misses=%u, lru_filled=%u)\n",
	      blkno, blru->blkno, lru_hits, lru_misses, lru_filled);
	}
      else
	{
	  XPR(NT "source block %"Q"u read (lru_hits=%u, lru_misses=%u, "
	      "lru_filled=%u)\n", blkno, lru_hits, lru_misses, lru_filled);
	}
    }

  blru->blkno      = blkno;
  source->curblk   = blru->blk;
  source->curblkno = blkno;
  source->onblk    = onblk;

  return 0;
}

/*********************************************************************
 Main routines
 ********************************************************************/

/* This is a generic input function.  It calls the xd3_encode_input or
 * xd3_decode_input functions and makes calls to the various input
 * handling routines above, which coordinate external decompression.
 */
static int
main_input (xd3_cmd     cmd,
	    main_file   *ifile,
	    main_file   *ofile,
	    main_file   *sfile)
{
  int        ret;
  xd3_stream stream;
  usize_t    nread;
  usize_t    winsize;
  int        stream_flags = 0;
  xd3_config config;
  xd3_source source;
  xoff_t     last_total_in = 0;
  xoff_t     last_total_out = 0;
  long       start_time;
  int        stdout_only = 0;
  int (*input_func) (xd3_stream*);
  int (*output_func) (xd3_stream*, main_file *);

  memset (& stream, 0, sizeof (stream));
  memset (& source, 0, sizeof (source));
  memset (& config, 0, sizeof (config));

  config.alloc = main_alloc;
  config.freef = main_free1;

  config.iopt_size = option_iopt_size;
  config.sprevsz = option_sprevsz;

  do_not_lru = 0;

  start_time = get_millisecs_now ();

  if (option_use_checksum) { stream_flags |= XD3_ADLER32; }

  /* main_input setup. */
  switch ((int) cmd)
    {
#if VCDIFF_TOOLS
           if (1) { case CMD_PRINTHDR:   stream_flags |= XD3_JUST_HDR; }
      else if (1) { case CMD_PRINTHDRS:  stream_flags |= XD3_SKIP_WINDOW; }
      else        { case CMD_PRINTDELTA: stream_flags |= XD3_SKIP_EMIT; }
      ifile->flags |= RD_NONEXTERNAL;
      input_func    = xd3_decode_input;
      output_func   = main_print_func;
      stream_flags |= XD3_ADLER32_NOVER;
      stdout_only   = 1;
      break;

    case CMD_RECODE:
    case CMD_MERGE:
    case CMD_MERGE_ARG:
      /* No source will be read */
      stream_flags |= XD3_ADLER32_NOVER | XD3_SKIP_EMIT;
      ifile->flags |= RD_NONEXTERNAL;
      input_func = xd3_decode_input;

      if ((ret = main_init_recode_stream ()))
        {
	  return EXIT_FAILURE;
        }

      if (cmd == CMD_RECODE) { output_func = main_recode_func; }
      else                   { output_func = main_merge_func; }
      break;
#endif /* VCDIFF_TOOLS */

#if XD3_ENCODER
    case CMD_ENCODE:
      do_not_lru  = 1;
      input_func  = xd3_encode_input;
      output_func = main_write_output;

      if (option_no_compress)      { stream_flags |= XD3_NOCOMPRESS; }
      if (option_use_altcodetable) { stream_flags |= XD3_ALT_CODE_TABLE; }
      if (option_smatch_config)
	{
	  char *s = option_smatch_config, *e;
	  int values[XD3_SOFTCFG_VARCNT];
	  int got;

	  config.smatch_cfg = XD3_SMATCH_SOFT;

	  for (got = 0; got < XD3_SOFTCFG_VARCNT; got += 1, s = e + 1)
	    {
	      values[got] = strtol (s, &e, 10);

	      if ((values[got] < 0) ||
		  (e == s) ||
		  (got < XD3_SOFTCFG_VARCNT-1 && *e == 0) ||
		  (got == XD3_SOFTCFG_VARCNT-1 && *e != 0))
		{
		  XPR(NT "invalid string match specifier (-C) %d: %s\n",
		      got, s);
		  return EXIT_FAILURE;
		}
	    }

	  config.smatcher_soft.large_look    = values[0];
	  config.smatcher_soft.large_step    = values[1];
	  config.smatcher_soft.small_look    = values[2];
	  config.smatcher_soft.small_chain   = values[3];
	  config.smatcher_soft.small_lchain  = values[4];
	  config.smatcher_soft.max_lazy      = values[5];
	  config.smatcher_soft.long_enough   = values[6];
	}
      else
	{
	  if (option_verbose > 1)
	    {
	      XPR(NT "compression level: %d\n", option_level);
	    }
	  if (option_level == 0)
	    {
	      stream_flags |= XD3_NOCOMPRESS;
	      config.smatch_cfg = XD3_SMATCH_FASTEST;
	    }
	  else if (option_level == 1)
	    { config.smatch_cfg = XD3_SMATCH_FASTEST; }
	  else if (option_level == 2)
	    { config.smatch_cfg = XD3_SMATCH_FASTER; }
	  else if (option_level <= 5)
	    { config.smatch_cfg = XD3_SMATCH_FAST; }
	  else if (option_level == 6)
	    { config.smatch_cfg = XD3_SMATCH_DEFAULT; }
	  else
	    { config.smatch_cfg = XD3_SMATCH_SLOW; }
	}
      break;
#endif
    case CMD_DECODE:
      if (option_use_checksum == 0) { stream_flags |= XD3_ADLER32_NOVER; }
      ifile->flags |= RD_NONEXTERNAL;
      input_func    = xd3_decode_input;
      output_func   = main_write_output;
      break;
    default:
      XPR(NT "internal error\n");
      return EXIT_FAILURE;
    }

  main_bsize = winsize = main_get_winsize (ifile);

  if ((main_bdata = (uint8_t*) main_malloc (winsize)) == NULL)
    {
      return EXIT_FAILURE;
    }

  if (IS_ENCODE (cmd))
    {
      /* When encoding, open the source file, possibly decompress it.
       * The decoder delays this step until XD3_GOTHEADER. */
      if (sfile->filename != NULL &&
	  (ret = main_set_source (NULL, cmd, sfile, & source)))
	{
	  return EXIT_FAILURE;
	}
    }

  config.winsize = winsize;
  config.srcwin_maxsz = option_srcwinsz;
  config.getblk = main_getblk_func;
  config.flags = stream_flags;

  if ((ret = main_set_secondary_flags (&config)) ||
      (ret = xd3_config_stream (& stream, & config)))
    {
      XPR(NT XD3_LIB_ERRMSG (& stream, ret));
      return EXIT_FAILURE;
    }

#if VCDIFF_TOOLS
  if ((cmd == CMD_MERGE || cmd == CMD_MERGE_ARG) && 
      (ret = xd3_whole_state_init (& stream)))
    {
      XPR(NT XD3_LIB_ERRMSG (& stream, ret));
      return EXIT_FAILURE;
    }
#endif

  if (IS_ENCODE (cmd) && sfile->filename != NULL &&
      (ret = xd3_set_source (& stream, & source)))
    {
      XPR(NT XD3_LIB_ERRMSG (& stream, ret));
      return EXIT_FAILURE;
    }

  /* This times each window. */
  get_millisecs_since ();

  /* Main input loop. */
  do
    {
      xoff_t input_offset;
      xoff_t input_remain;
      usize_t try_read;

      input_offset = ifile->nread;

      input_remain = XOFF_T_MAX - input_offset;

      try_read = (usize_t) min ((xoff_t) config.winsize, input_remain);

      if ((ret = main_read_primary_input (ifile, main_bdata,
					  try_read, & nread)))
	{
	  return EXIT_FAILURE;
	}

      /* If we've reached EOF tell the stream to flush. */
      if (nread < try_read)
	{
	  stream.flags |= XD3_FLUSH;
	}

#if XD3_ENCODER
      /* After the first main_read_primary_input completes, we know
       * all the information needed to encode the application
       * header. */
      if (cmd == CMD_ENCODE &&
	  (ret = main_set_appheader (& stream, ifile, sfile)))
	{
	  return EXIT_FAILURE;
	}
#endif
      xd3_avail_input (& stream, main_bdata, nread);

      /* If we read zero bytes after encoding at least one window... */
      if (nread == 0 && stream.current_window > 0) {
	break;
      }

    again:
      ret = input_func (& stream);

      switch (ret)
	{
	case XD3_INPUT:
	  continue;

	case XD3_GOTHEADER:
	  {
	    XD3_ASSERT (stream.current_window == 0);

	    /* Need to process the appheader as soon as possible.  It may
	     * contain a suggested default filename/decompression routine for
	     * the ofile, and it may contain default/decompression routine for
	     * the sources. */
	    if (cmd == CMD_DECODE)
	      {
		/* May need to set the sfile->filename if none was given. */
		main_get_appheader (& stream, ifile, ofile, sfile);

		/* Now open the source file. */
		  if ((sfile->filename != NULL) &&
		      (ret = main_set_source (& stream, cmd, sfile, & source)))
		  {
		    return EXIT_FAILURE;
		  }
	      }
	    else if (cmd == CMD_PRINTHDR ||
		     cmd == CMD_PRINTHDRS ||
		     cmd == CMD_PRINTDELTA ||
		     cmd == CMD_RECODE)
	      {
		if (sfile->filename == NULL)
		  {
		    allow_fake_source = 1;
		    sfile->filename = "<placeholder>";
		    main_set_source (& stream, cmd, sfile, & source);
		  }
	      }
	  }
	/* FALLTHROUGH */
	case XD3_WINSTART:
	  {
	    /* e.g., set or unset XD3_SKIP_WINDOW. */
	    goto again;
	  }

	case XD3_OUTPUT:
	  {
	    /* Defer opening the output file until the stream produces its
	     * first output for both encoder and decoder, this way we
	     * delay long enough for the decoder to receive the
	     * application header.  (Or longer if there are skipped
	     * windows, but I can't think of any reason not to delay
	     * open.) */
	    if (ofile != NULL &&
		! main_file_isopen (ofile) &&
		(ret = main_open_output (& stream, ofile)) != 0)
	      {
		return EXIT_FAILURE;
	      }
	    
	    if ((ret = output_func (& stream, ofile)) &&
		(ret != PRINTHDR_SPECIAL))
	      {
		return EXIT_FAILURE;
	      }

	    if (ret == PRINTHDR_SPECIAL)
	      {
		xd3_abort_stream (& stream);
		ret = EXIT_SUCCESS;
		goto done;
	      }

	    ret = 0;

	    xd3_consume_output (& stream);
	    goto again;
	  }

	case XD3_WINFINISH:
	  {
	    if (IS_ENCODE (cmd) || cmd == CMD_DECODE || cmd == CMD_RECODE)
	      {
		if (! option_quiet && IS_ENCODE (cmd) &&
		    main_file_isopen (sfile))
		  {
		    /* Warn when no source copies are found */
		    if (option_verbose && ! xd3_encoder_used_source (& stream))
		      {
			XPR(NT "warning: input window %"Q"u..%"Q"u has "
			    "no source copies\n",
			    stream.current_window * winsize,
			    (stream.current_window+1) * winsize);
		      }

		    /* Limited i-buffer size affects source copies */
		    if (option_verbose > 1 &&
			stream.i_slots_used > stream.iopt_size)
		      {
			XPR(NT "warning: input position %"Q"u overflowed "
			    "instruction buffer, needed %u (vs. %u), "
			    "consider raising -I\n",
			    stream.current_window * winsize,
			    stream.i_slots_used, stream.iopt_size);
		      }
		  }

		if (option_verbose)
		  {
		    char rrateavg[32], wrateavg[32], tm[32];
		    char rdb[32], wdb[32];
		    char trdb[32], twdb[32];
		    long millis = get_millisecs_since ();
		    usize_t this_read = (usize_t)(stream.total_in -
						  last_total_in);
		    usize_t this_write = (usize_t)(stream.total_out -
						   last_total_out);
		    last_total_in = stream.total_in;
		    last_total_out = stream.total_out;

		    if (option_verbose > 1)
		      {
			XPR(NT "%"Q"u: in %s (%s): out %s (%s): "
			    "total in %s: out %s: %s\n",
			    stream.current_window,
			    main_format_bcnt (this_read, rdb),
			    main_format_rate (this_read, millis, rrateavg),
			    main_format_bcnt (this_write, wdb),
			    main_format_rate (this_write, millis, wrateavg),
			    main_format_bcnt (stream.total_in, trdb),
			    main_format_bcnt (stream.total_out, twdb),
			    main_format_millis (millis, tm));
		      }
		    else
		      {
			XPR(NT "%"Q"u: in %s: out %s: total in %s: "
			    "out %s: %s\n",
 			    stream.current_window,
			    main_format_bcnt (this_read, rdb),
			    main_format_bcnt (this_write, wdb),
			    main_format_bcnt (stream.total_in, trdb),
			    main_format_bcnt (stream.total_out, twdb),
			    main_format_millis (millis, tm));
		      }
		  }
	      }
	    goto again;
	  }

	default:
	  /* input_func() error */
	  XPR(NT XD3_LIB_ERRMSG (& stream, ret));
	  return EXIT_FAILURE;
	}
    }
  while (nread == config.winsize);
done:
  /* Close the inputs. (ifile must be open, sfile may be open) */
  main_file_close (ifile);
  if (sfile != NULL)
    {
      main_file_close (sfile);
    }

#if VCDIFF_TOOLS
  if (cmd == CMD_MERGE &&
      (ret = main_merge_output (& stream, ofile)))
    {
      return EXIT_FAILURE;
    }

  if (cmd == CMD_MERGE_ARG)
    {
      xd3_swap_whole_state (& stream.whole_target,
			    & recode_stream->whole_target);
    }
#endif /* VCDIFF_TOOLS */

  /* If output file is not open yet because of delayed-open, it means
   * we never encountered a window in the delta, but it could have had
   * a VCDIFF header?  TODO: solve this elsewhere.  For now, it prints
   * "nothing to output" below, but the check doesn't happen in case
   * of option_no_output.  */
  if (! option_no_output && ofile != NULL)
    {
      if (!stdout_only && ! main_file_isopen (ofile))
	{
	  XPR(NT "nothing to output: %s\n", ifile->filename);
	  return EXIT_FAILURE;
	}

      /* Have to close the output before calling
       * main_external_compression_finish, or else it hangs. */
      if (main_file_close (ofile) != 0)
	{
	  return EXIT_FAILURE;
	}
    }

#if EXTERNAL_COMPRESSION
  if ((ret = main_external_compression_finish ()))
    {
      XPR(NT "external compression commands failed\n");
      return EXIT_FAILURE;
    }
#endif

  if ((ret = xd3_close_stream (& stream)))
    {
      XPR(NT XD3_LIB_ERRMSG (& stream, ret));
      return EXIT_FAILURE;
    }

#if XD3_ENCODER
  if (option_verbose > 1 && cmd == CMD_ENCODE)
    {
      XPR(NT "scanner configuration: %s\n", stream.smatcher.name);
      XPR(NT "target hash table size: %u\n", stream.small_hash.size);
      if (sfile != NULL && sfile->filename != NULL)
	{
	  XPR(NT "source hash table size: %u\n", stream.large_hash.size);
	}
    }

  if (option_verbose > 2 && cmd == CMD_ENCODE)
    {
      XPR(NT "source copies: %"Q"u (%"Q"u bytes)\n",
	  stream.n_scpy, stream.l_scpy);
      XPR(NT "target copies: %"Q"u (%"Q"u bytes)\n",
	  stream.n_tcpy, stream.l_tcpy);
      XPR(NT "adds: %"Q"u (%"Q"u bytes)\n", stream.n_add, stream.l_add);
      XPR(NT "runs: %"Q"u (%"Q"u bytes)\n", stream.n_run, stream.l_run);
    }
#endif

  xd3_free_stream (& stream);

  if (option_verbose)
    {
      char tm[32];
      long end_time = get_millisecs_now ();
      xoff_t nwrite = ofile != NULL ? ofile->nwrite : 0;

      XPR(NT "finished in %s; input %"Q"u  output %"Q"u bytes  (%0.2f%%)\n",
	  main_format_millis (end_time - start_time, tm),
	  ifile->nread, nwrite, 100.0 * nwrite / ifile->nread);
    }

  return EXIT_SUCCESS;
}

/* free memory before exit, reset single-use variables. */
static void
main_cleanup (void)
{
  usize_t i;

  if (appheader_used != NULL &&
      appheader_used != option_appheader)
    {
      main_free (appheader_used);
      appheader_used = NULL;
    }

  main_free (main_bdata);
  main_bdata = NULL;
  main_bsize = 0;

#if EXTERNAL_COMPRESSION
  main_free (ext_tmpfile);
  ext_tmpfile = NULL;
#endif

  for (i = 0; lru && i < lru_size; i += 1)
    {
      main_free (lru[i].blk);
    }

  main_free (lru);
  lru = NULL;

  lru_hits = 0;
  lru_misses = 0;
  lru_filled = 0;

  if (recode_stream != NULL)
    {
      xd3_free_stream (recode_stream);
      main_free (recode_stream);
      recode_stream = NULL;
    }

  if (merge_stream != NULL)
    {
      xd3_free_stream (merge_stream);
      main_free (merge_stream);
      merge_stream = NULL;
    }

  XD3_ASSERT (main_mallocs == 0);
}

static void
setup_environment (int argc,
		   char **argv,
		   int *argc_out,
		   char ***argv_out,
		   char ***argv_free,
		   char **env_free)
{
  int n, i, i0;
  char *p, *v = getenv("XDELTA");
  if (v == NULL) {
    (*argc_out) = argc;
    (*argv_out) = argv;
    (*argv_free) = NULL;
    (*env_free) = NULL;
    return;
  }

  (*env_free) = (char*) main_malloc(strlen(v) + 1);
  strcpy(*env_free, v);

  /* Space needed for extra args, at least # of spaces */
  n = argc + 1;
  for (p = *env_free; *p != 0; ) {
    if (*p++ == ' ') {
      n++;
    }
  }

  (*argv_free) = (char**) main_malloc(sizeof(char*) * (n + 1));
  (*argv_out) = (*argv_free);
  (*argv_out)[0] = argv[0];
  (*argv_out)[n] = NULL;

  i = 1;
  for (p = *env_free; *p != 0; ) {
    (*argv_out)[i++] = p;
    while (*p != ' ' && *p != 0) {
      p++;
    }
    while (*p == ' ') {
      *p++ = 0;
    }
  }

  for (i0 = 1; i0 < argc; i0++) {
    (*argv_out)[i++] = argv[i0];
  }

  /* Counting spaces is an upper bound, argv stays NULL terminated. */
  (*argc_out) = i;
  while (i <= n) {
    (*argv_out)[i++] = NULL;
  }
}

int
#if PYTHON_MODULE || SWIG_MODULE || NOT_MAIN
xd3_main_cmdline (int argc, char **argv)
#else
main (int argc, char **argv)
#endif
{
  static const char *flags =
    "0123456789cdefhnqvDJNORTVs:m:B:C:E:F:I:L:O:M:P:W:A::S::";
  xd3_cmd cmd;
  main_file ifile;
  main_file ofile;
  main_file sfile;
  main_merge_list merge_order;
  main_merge *merge;
  int my_optind;
  char *my_optarg;
  char *my_optstr;
  char *sfilename;
  int env_argc;
  char **env_argv;
  char **free_argv;  /* malloc() in setup_environment() */
  char *free_value;  /* malloc() in setup_environment() */
  int ret;

#ifdef _WIN32
  GetStartupInfo(&winStartupInfo);
  setvbuf(stderr, NULL, _IONBF, 0);  /* Do not buffer stderr */
#endif

  main_file_init (& ifile);
  main_file_init (& ofile);
  main_file_init (& sfile);
  main_merge_list_init (& merge_order);

  reset_defaults();

  free_argv = NULL;
  free_value = NULL;
  setup_environment(argc, argv, &env_argc, &env_argv,
		    &free_argv, &free_value);
  cmd = CMD_NONE;
  sfilename = NULL;
  my_optind = 1;
  argv = env_argv;
  argc = env_argc;
  program_name = env_argv[0];
  extcomp_types[0].recomp_cmdname = program_name;
  extcomp_types[0].decomp_cmdname = program_name;

 takearg:
  my_optarg = NULL;
  my_optstr = argv[my_optind];

  /* This doesn't use getopt() because it makes trouble for -P & python which
   * reenter main() and thus care about freeing all memory.  I never had much
   * trust for getopt anyway, it's too opaque.  This implements a fairly
   * standard non-long-option getopt with support for named operations (e.g.,
   * "xdelta3 [encode|decode|printhdr...] < in > out"). */
  if (my_optstr)
    {
      if (*my_optstr == '-')    { my_optstr += 1; }
      else if (cmd == CMD_NONE) { goto nonflag; }
      else                      { my_optstr = NULL; }
    }
  while (my_optstr)
    {
      char *s;
      my_optarg = NULL;
      if ((ret = *my_optstr++) == 0) { my_optind += 1; goto takearg; }

      /* Option handling: first check for one ':' following the option in
       * flags, then check for two.  The syntax allows:
       *
       * 1. -Afoo                   defines optarg="foo"
       * 2. -A foo                  defines optarg="foo"
       * 3. -A ""                   defines optarg="" (allows empty-string)
       * 4. -A [EOA or -moreargs]   error (mandatory case)
       * 5. -A [EOA -moreargs]      defines optarg=NULL (optional case)
       * 6. -A=foo                  defines optarg="foo"
       * 7. -A=                     defines optarg="" (mandatory case)
       * 8. -A=                     defines optarg=NULL (optional case)
       *
       * See tests in test_command_line_arguments().
       */
      s = strchr (flags, ret);
      if (s && s[1] && s[1] == ':')
	{
	  int eqcase = 0;
	  int option = s[2] && s[2] == ':';

	  /* Case 1, set optarg to the remaining characters. */
	  my_optarg = my_optstr;
	  my_optstr = "";

	  /* Case 2-5 */
	  if (*my_optarg == 0)
	    {
	      /* Condition 4-5 */
	      int have_arg = (my_optind < (argc - 1) &&
			      *argv[my_optind+1] != '-');

	      if (! have_arg)
		{
		  if (! option)
		  {
		    /* Case 4 */
		    XPR(NT "-%c: requires an argument\n", ret);
		    ret = EXIT_FAILURE;
		    goto cleanup;
		  }
		  /* Case 5. */
		  my_optarg = NULL;
		}
	      else
		{
		  /* Case 2-3. */
		  my_optarg = argv[++my_optind];
		}
	    }
	  /* Case 6-8. */
	  else if (*my_optarg == '=')
	    {
	      /* Remove the = in all cases. */
	      my_optarg += 1;
	      eqcase = 1;

	      if (option && *my_optarg == 0)
		{
		  /* Case 8. */
		  my_optarg = NULL;
		}
	    }
	}

      switch (ret)
	{
	/* case: if no '-' was found, maybe check for a command name. */
	nonflag:
	       if (strcmp (my_optstr, "decode") == 0) { cmd = CMD_DECODE; }
	  else if (strcmp (my_optstr, "encode") == 0)
	    {
#if XD3_ENCODER
	      cmd = CMD_ENCODE;
#else
	      XPR(NT "encoder support not compiled\n");
	      return EXIT_FAILURE;
#endif
	    }
	  else if (strcmp (my_optstr, "config") == 0) { cmd = CMD_CONFIG; }
#if REGRESSION_TEST
	  else if (strcmp (my_optstr, "test") == 0) { cmd = CMD_TEST; }
#endif
#if VCDIFF_TOOLS
	  else if (strcmp (my_optstr, "printhdr") == 0) { cmd = CMD_PRINTHDR; }
	  else if (strcmp (my_optstr, "printhdrs") == 0)
	    { cmd = CMD_PRINTHDRS; }
	  else if (strcmp (my_optstr, "printdelta") == 0)
	    { cmd = CMD_PRINTDELTA; }
	  else if (strcmp (my_optstr, "recode") == 0) { cmd = CMD_RECODE; }
	  else if (strcmp (my_optstr, "merge") == 0) { cmd = CMD_MERGE; }
#endif

	  /* If no option was found and still no command, let the default
	   * command be encode.  The remaining args are treated as
	   * filenames. */
	  if (cmd == CMD_NONE)
	    {
	      cmd = CMD_DEFAULT;
	      my_optstr = NULL;
	      break;
	    }
	  else
	    {
	      /* But if we find a command name, continue the getopt loop. */
	      my_optind += 1;
	      goto takearg;
	    }

	  /* gzip-like options */
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  option_level = ret - '0';
	  break;
	case 'f': option_force = 1; break;
	case 'v': option_verbose += 1; option_quiet = 0; break;
	case 'q': option_quiet = 1; option_verbose = 0; break;
	case 'c': option_stdout = 1; break;
	case 'd':
	  if (cmd == CMD_NONE) { cmd = CMD_DECODE; }
	  else { ret = main_help (); goto exit; }
	  break;
	case 'e':
#if XD3_ENCODER
	  if (cmd == CMD_NONE) { cmd = CMD_ENCODE; }
	  else { ret = main_help (); goto exit; }
	  break;
#else
	  XPR(NT "encoder support not compiled\n");
	  return EXIT_FAILURE;
#endif

	case 'n': option_use_checksum = 0; break;
	case 'N': option_no_compress = 1; break;
	case 'T': option_use_altcodetable = 1; break;
	case 'C': option_smatch_config = my_optarg; break;
	case 'J': option_no_output = 1; break;
	case 'S': if (my_optarg == NULL)
	    {
	      option_use_secondary = 1;
	      option_secondary = "none";
	    }
	  else
	    {
	      option_use_secondary = 1;
	      option_secondary = my_optarg;
	    }
	  break;
	case 'A': if (my_optarg == NULL) { option_use_appheader = 0; }
	          else { option_appheader = (uint8_t*) my_optarg; } break;
	case 'B':
	  if ((ret = main_atou (my_optarg, & option_srcwinsz, XD3_MINSRCWINSZ,
				0, 'B')))
	    {
	      goto exit;
	    }
	  break;
	case 'I':
	  if ((ret = main_atou (my_optarg, & option_iopt_size, 0,
				0, 'I')))
	    {
	      goto exit;
	    }
	  break;
	case 'P':
	  if ((ret = main_atou (my_optarg, & option_sprevsz, 0,
				0, 'P')))
	    {
	      goto exit;
	    }
	  break;
	case 'W':
	  if ((ret = main_atou (my_optarg, & option_winsize, XD3_ALLOCSIZE,
				XD3_HARDMAXWINSIZE, 'W')))
	  {
	    goto exit;
	  }
	  break;
	case 'D':
#if EXTERNAL_COMPRESSION == 0
	  if (option_verbose > 0)
	    {
	      XPR(NT "warning: -D option ignored, "
		       "external compression support was not compiled\n");
	    }
#else
	  option_decompress_inputs  = 0;
#endif
	  break;
	case 'R':
#if EXTERNAL_COMPRESSION == 0
	  if (option_verbose > 0)
	    {
	      XPR(NT "warning: -R option ignored, "
		       "external compression support was not compiled\n");
	    }
#else
	  option_recompress_outputs = 0;
#endif
	  break;
	case 's':
	  if (sfilename != NULL)
	    {
	      XPR(NT "specify only one source file\n");
	      goto cleanup;
	    }

	  sfilename = my_optarg;
	  break;
	case 'm':
	  if ((merge = (main_merge*)
	       main_malloc (sizeof (main_merge))) == NULL)
	    {
	      goto cleanup;
	    }
	  main_merge_list_push_back (& merge_order, merge);
	  merge->filename = my_optarg;
	  break;
	case 'V':
	  ret = main_version (); goto exit;
	default:
	  ret = main_help (); goto exit;
	}
    }

  option_source_filename = sfilename;

  /* In case there were no arguments, set the default command. */
  if (cmd == CMD_NONE) { cmd = CMD_DEFAULT; }

  argc -= my_optind;
  argv += my_optind;

  /* There may be up to two more arguments. */
  if (argc > 2)
    {
      XPR(NT "too many filenames: %s ...\n", argv[2]);
      goto cleanup;
    }

  ifile.flags    = RD_FIRST;
  sfile.flags    = RD_FIRST;
  sfile.filename = option_source_filename;

  /* The infile takes the next argument, if there is one.  But if not, infile
   * is set to stdin. */
  if (argc > 0)
    {
      ifile.filename = argv[0];

      if ((ret = main_file_open (& ifile, ifile.filename, XO_READ)))
	{
	  goto cleanup;
	}
    }
  else
    {
      XSTDIN_XF (& ifile);
    }

  /* The ofile takes the following argument, if there is one.  But if not, it
   * is left NULL until the application header is processed.  It will be set
   * in main_open_output. */
  if (argc > 1)
    {
      /* Check for conflicting arguments. */
      if (option_stdout && ! option_quiet)
	{
	  XPR(NT "warning: -c option overrides output filename: %s\n",
	      argv[1]);
	}

      if (! option_stdout) { ofile.filename = argv[1]; }
    }

#if VCDIFF_TOOLS
  if (cmd == CMD_MERGE &&
      (ret = main_merge_arguments (&merge_order)))
    {
      goto cleanup;
    }
#endif /* VCDIFF_TOOLS */

  switch (cmd)
    {
    case CMD_PRINTHDR:
    case CMD_PRINTHDRS:
    case CMD_PRINTDELTA:
#if XD3_ENCODER
    case CMD_ENCODE:
    case CMD_RECODE:
    case CMD_MERGE:
#endif
    case CMD_DECODE:
      ret = main_input (cmd, & ifile, & ofile, & sfile);
      break;

#if REGRESSION_TEST
    case CMD_TEST:
      main_config ();
      ret = xd3_selftest ();
      break;
#endif

    case CMD_CONFIG:
      ret = main_config ();
      break;

    default:
      ret = main_help ();
      break;
    }

  if (0)
    {
    cleanup:
      ret = EXIT_FAILURE;
    exit:
      (void)0;
    }

#if EXTERNAL_COMPRESSION
  if (ext_tmpfile != NULL)
    {
      unlink (ext_tmpfile);
    }
#endif

  main_file_cleanup (& ifile);
  main_file_cleanup (& ofile);
  main_file_cleanup (& sfile);

  while (! main_merge_list_empty (& merge_order))
    {
      merge = main_merge_list_pop_front (& merge_order);
      main_free (merge);
    }

  main_free (free_argv);
  main_free (free_value);

  main_cleanup ();

  fflush (stdout);
  fflush (stderr);
  return ret;
}

static int
main_help (void)
{
  main_version();

  /* Note: update wiki when command-line features change */
  DP(RINT "usage: xdelta3 [command/options] [input [output]]\n");
  DP(RINT "special command names:\n");
  DP(RINT "    config      prints xdelta3 configuration\n");
  DP(RINT "    decode      decompress the input\n");
  DP(RINT "    encode      compress the input%s\n",
     XD3_ENCODER ? "" : " [Not compiled]");
#if REGRESSION_TEST
  DP(RINT "    test        run the builtin tests\n");
#endif
#if VCDIFF_TOOLS
  DP(RINT "special commands for VCDIFF inputs:\n");
  DP(RINT "    printdelta  print information about the entire delta\n");
  DP(RINT "    printhdr    print information about the first window\n");
  DP(RINT "    printhdrs   print information about all windows\n");
  DP(RINT "    recode      encode with new application/secondary settings\n");
  DP(RINT "    merge       merge VCDIFF inputs (see below)\n");
#endif
  DP(RINT "standard options:\n");
  DP(RINT "   -0 .. -9     compression level\n");
  DP(RINT "   -c           use stdout\n");
  DP(RINT "   -d           decompress\n");
  DP(RINT "   -e           compress%s\n",
     XD3_ENCODER ? "" : " [Not compiled]");
  DP(RINT "   -f           force overwrite\n");
  DP(RINT "   -h           show help\n");
  DP(RINT "   -q           be quiet\n");
  DP(RINT "   -v           be verbose (max 2)\n");
  DP(RINT "   -V           show version\n");

  DP(RINT "memory options:\n");
  DP(RINT "   -B bytes     source window size\n");
  DP(RINT "   -W bytes     input window size\n");
  DP(RINT "   -P size      compression duplicates window\n");
  DP(RINT "   -I size      instruction buffer size (0 = unlimited)\n");

  DP(RINT "compression options:\n");
  DP(RINT "   -s source    source file to copy from (if any)\n");
  DP(RINT "   -S [djw|fgk] enable/disable secondary compression\n");
  DP(RINT "   -N           disable small string-matching compression\n");
  DP(RINT "   -D           disable external decompression (encode/decode)\n");
  DP(RINT "   -R           disable external recompression (decode)\n");
  DP(RINT "   -n           disable checksum (encode/decode)\n");
  DP(RINT "   -C           soft config (encode, undocumented)\n");
  DP(RINT "   -A [apphead] disable/provide application header (encode)\n");
  DP(RINT "   -J           disable output (check/compute only)\n");
  DP(RINT "   -T           use alternate code table (test)\n");
  DP(RINT "   -m           arguments for \"merge\"\n");

  DP(RINT "the XDELTA environment variable may contain extra args:\n");
  DP(RINT "   XDELTA=\"-s source-x.y.tar.gz\" \\\n");
  DP(RINT "   tar --use-compress-program=xdelta3 \\\n");
  DP(RINT "       -cf target-x.z.tar.gz.vcdiff target-x.y\n");
  DP(RINT "the \"merge\" command combines VCDIFF inputs as follows:\n");
  DP(RINT "   xdelta3 merge -m 1.vcdiff -m 2.vcdiff 3.vcdiff merged.vcdiff\n");
  return EXIT_FAILURE;
}
