/* -*- Mode: C;-*-
 *
 * This file is part of XDelta - A binary delta generator.
 *
 * Copyright (C) 1997, 1998, 1999, 2001  Josh MacDonald
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 *
 * $Id: xdmain.c 1.22.1.7.1.1 Sat, 27 Jan 2007 17:53:47 -0800 jmacd $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <fcntl.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(__DJGPP__)
#define WINHACK
#endif

#ifndef WINHACK
#include <unistd.h>
#include <sys/mman.h>
// #define O_BINARY 0

#else /* WINHACK */

#include <io.h>
#include <process.h>
#ifdef __DJGPP__
#include <unistd.h>
#include <dpmi.h>
#endif

#define STDOUT_FILENO 1
#define lstat stat

#ifndef __DJGPP__
#define S_IFMT _S_IFMT
#define S_IFREG _S_IFREG
#endif /* !__DJGPP__ */

#endif /* !WINHACK_ */

#include <zlib.h>

#include "xdelta.h"

static HandleFuncTable xd_handle_table;

#define XD_PAGE_SIZE (1<<20)

#define XDELTA_110_PREFIX "%XDZ004%"
#define XDELTA_104_PREFIX "%XDZ003%"
#define XDELTA_100_PREFIX "%XDZ002%"
#define XDELTA_020_PREFIX "%XDZ001%"
#define XDELTA_018_PREFIX "%XDZ000%"
#define XDELTA_014_PREFIX "%XDELTA%"
#define XDELTA_PREFIX     XDELTA_110_PREFIX
#define XDELTA_PREFIX_LEN 8

#define HEADER_WORDS (6)
#define HEADER_SPACE (HEADER_WORDS*4)
/* The header is composed of 4-byte words (network byte order) as follows:
 * word 1: flags
 * word 2: (from name length) << 16 | (to name length)
 * word 3: (reserved)
 * word 4: (reserved)
 * word 5: (reserved)
 * word 6: (reserved)
 * flags are:
 */
#define FLAG_NO_VERIFY 1
#define FLAG_FROM_COMPRESSED 2
#define FLAG_TO_COMPRESSED 4
#define FLAG_PATCH_COMPRESSED 8
/* and, the header is follwed by the from file name, then the to file
 * name, then the data. */

#ifdef WINHACK
#define FOPEN_READ_ARG "rb"
#define FOPEN_WRITE_ARG "wb"
#define FILE_SEPARATOR '\\'
#else
#define FOPEN_READ_ARG "r"
#define FOPEN_WRITE_ARG "w"
#define FILE_SEPARATOR '/'
#endif /* WINHACK */

#include "getopt.h"

typedef struct _LRU LRU;

struct _LRU
{
  LRU *next;
  LRU *prev;

  gint refs;
  guint page;
  guint8* buffer;
};

typedef struct _XdFileHandle XdFileHandle;

typedef struct {
  gboolean       patch_is_compressed;
  const gchar*   patch_name;
  guint          patch_flags;
  const gchar*   patch_version;
  gboolean       has_trailer;

  XdeltaSourceInfo* data_source;
  XdeltaSourceInfo* from_source;

  gchar         *from_name;
  gchar         *to_name;

  guint          control_offset;
  guint          header_offset;

  gint16         from_name_len;
  gint16         to_name_len;

  guint32        header_space[HEADER_WORDS];
  guint8         magic_buf[XDELTA_PREFIX_LEN];

  XdFileHandle      *patch_in;

  XdeltaControl   *cont;
} XdeltaPatch;

struct _XdFileHandle
{
  FileHandle fh;

  guint    length;
  guint    real_length;
  gint     type;
  const char* name;
  const char* cleanup;

  guint8 md5[16];
  EdsioMD5Ctx ctx;

  /* for write */
  int out_fd;
  void* out;
  gboolean (* out_write) (XdFileHandle* handle, const void* buf, gint nbyte);
  gboolean (* out_close) (XdFileHandle* handle);

  /* for read */
  GPtrArray *lru_table;
  LRU       *lru_head;  /* most recently used. */
  LRU       *lru_tail;  /* least recently used. */
  GMemChunk *lru_chunk;
  guint      lru_count;
  guint      lru_outstanding_refs;

  guint    narrow_low;
  guint    narrow_high;
  guint    current_pos;
  FILE*    in;
  gboolean (* in_read) (XdFileHandle* handle, void* buf, gint nbyte);
  gboolean (* in_close) (XdFileHandle* handle);
  gboolean in_compressed;

  const guint8* copy_page;
  guint  copy_pgno;
  gboolean md5_good;
  gboolean reset_length_next_write;

  gint md5_page;
  gint fd;
};

/* $Format: "static const char xdelta_version[] = \"$ReleaseVersion$\"; " $ */
static const char xdelta_version[] = "1.1.4"; 

typedef struct _Command Command;

struct _Command {
  gchar* name;
  gint (* func) (gint argc, gchar** argv);
  gint nargs;
};

static gint    delta_command    (gint argc, gchar** argv);
static gint    patch_command    (gint argc, gchar** argv);
static gint    info_command     (gint argc, gchar** argv);

static const Command commands[] =
{
  { "delta",    delta_command,    -1 },
  { "patch",    patch_command,    -1 },
  { "info",     info_command,     1 },
  { NULL, NULL, 0 }
};

static struct option const long_options[] =
{
  {"help",                no_argument, 0, 'h'},
  {"version",             no_argument, 0, 'v'},
  {"verbose",             no_argument, 0, 'V'},
  {"noverify",            no_argument, 0, 'n'},
  {"pristine",            no_argument, 0, 'p'},
  {"quiet",               no_argument, 0, 'q'},
  {"maxmem",              required_argument, 0, 'm'},
  {"blocksize",           required_argument, 0, 's'},
  {0,0,0,0}
};

static const gchar* program_name;
static gint         compress_level = Z_DEFAULT_COMPRESSION;
static gint         no_verify = FALSE;
static gint         pristine = FALSE;
static gint         verbose = FALSE;
static gint         max_mapped_pages = G_MAXINT;
static gint         quiet = FALSE;

#define xd_error g_warning

static void
usage ()
{
  xd_error ("usage: %s COMMAND [OPTIONS] [ARG1 ...]\n", program_name);
  xd_error ("use --help for more help\n");
  exit (2);
}

static void
help ()
{
  xd_error ("usage: %s COMMAND [OPTIONS] [ARG1 ...]\n", program_name);
  xd_error ("COMMAND is one of:\n");
  xd_error ("  delta     Produce a delta from ARG1 to ARG2 producing ARG3\n");
  xd_error ("  info      List details about delta ARG1\n");
  xd_error ("  patch     Apply patch ARG1 using file ARG2 producing ARG3\n");
  xd_error ("OPTIONS are:\n");
  xd_error ("  -v, --version      Print version information\n");
  xd_error ("  -V, --verbose      Print verbose error messages\n");
  xd_error ("  -h, --help         Print this summary\n");
  xd_error ("  -n, --noverify     Disable automatic MD5 verification\n");
  xd_error ("  -p, --pristine     Disable automatic GZIP decompression\n");
  xd_error ("  -q, --quiet        Do not print warnings\n");
  xd_error ("  -m, --maxmem=SIZE  Set the buffer size limit, e.g. 640K, 16M\n");
  xd_error ("  -[0-9]             ZLIB compress level: 0=none, 1=fast, 6=default, 9=best\n");
  xd_error ("  -s=BLOCK_SIZE      Sets block size (power of 2), minimum match length\n");
  xd_error ("                     In-core memory requirement is (FROM_LEN * 8) / BLOCK_SIZE\n");
  exit (2);
}

static void
version ()
{
  xd_error ("version %s\n", xdelta_version);
  exit (2);
}

static FILE* xd_error_file = NULL;

static void
xd_error_func (const gchar   *log_domain,
	       GLogLevelFlags	log_level,
	       const gchar   *message,
	       gpointer	user_data)
{
  if (! xd_error_file)
    xd_error_file = stderr;

  fprintf (xd_error_file, "%s: %s", program_name, message);
}

static gboolean
event_devel (void)
{
  static gboolean once = FALSE;
  static gboolean devel = FALSE;

  if (! once)
    {
      devel = g_getenv ("EDSIO_DEVEL") != NULL;
      once = TRUE;
    }

  return devel;
}

static gboolean
event_watch (GenericEvent* ev, GenericEventDef* def, const char* message)
{
  if (quiet && def->level <= EL_Warning)
    return TRUE;

  if (event_devel ())
    fprintf (stderr, "%s:%d: %s\n", ev->srcfile, ev->srcline, message);
  else
    fprintf (stderr, "%s: %s\n", program_name, message);

  return TRUE;
}

gint
main (gint argc, gchar** argv)
{
  const Command *cmd = NULL;
  gint c;
  gint longind;

  eventdelivery_event_watch_all (event_watch);

  if (! xd_edsio_init ())
    return 2;

#ifdef __DJGPP__
  /*
   * (richdawe@bigfoot.com): Limit maximum memory usage to 87.5% of available
   * physical memory for DJGPP. Otherwise the replacement for mmap() will
   * exhaust memory and XDelta will fail.
   */
  {
    unsigned long phys_free = _go32_dpmi_remaining_physical_memory();

    max_mapped_pages  = phys_free / XD_PAGE_SIZE;
    max_mapped_pages %= 8;
    max_mapped_pages *= 7;
  }

  /*
   * (richdawe@bigfoot.com): Strip off the file extension 'exe' from
   * program_name, since it makes the help messages look ugly.
   */
  {
    char strip_ext[PATH_MAX + 1];
    char *p;

    strcpy (strip_ext, argv[0]);
    p = strrchr (strip_ext, '.');
    if ((p != NULL) && (strncasecmp (p + 1, "exe", 3) == 0))
      *p = '\0';
    program_name = g_basename (strip_ext);
  }
#else /* !__DJGPP__ */
  program_name = g_basename (argv[0]);
#endif /* __DJGPP__ */  

  g_log_set_handler (G_LOG_DOMAIN,
		     G_LOG_LEVEL_WARNING,
		     xd_error_func,
		     NULL);

  if (argc < 2)
    usage ();

  for (cmd = commands; cmd->name; cmd += 1)
    if (strcmp (cmd->name, argv[1]) == 0)
      break;

  if (strcmp (argv[1], "-h") == 0 ||
      strcmp (argv[1], "--help") == 0)
    help ();

  if (strcmp (argv[1], "-v") == 0 ||
      strcmp (argv[1], "--version") == 0)
    version ();

  if (!cmd->name)
    {
      xd_error ("unrecognized command\n");
      help ();
    }

  argc -= 1;
  argv += 1;

  while ((c = getopt_long(argc,
			  argv,
			  "+nqphvVs:m:0123456789",
			  long_options,
			  &longind)) != EOF)
    {
      switch (c)
	{
	case 'q': quiet = TRUE; break;
	case 'n': no_verify = TRUE; break;
	case 'p': pristine = TRUE; break;
	case 'V': verbose = TRUE; break;
	case 's':
	  {
	    int ret;
	    int s = atoi (optarg);

	    if ((ret = xdp_set_query_size_pow (s)) && ! quiet)
	      {
		xd_error ("illegal query size: %s\n", xdp_errno (ret));
	      }
	  }
	  break;
	case 'm':
	  {
	    gchar* end = NULL;
	    glong l = strtol (optarg, &end, 0);

	    if (end && g_strcasecmp (end, "M") == 0)
	      l <<= 20;
	    else if (end && g_strcasecmp (end, "K") == 0)
	      l <<= 10;
	    else if (end || l < 0)
	      {
		xd_error ("illegal maxmem argument %s\n", optarg);
		return 2;
	      }

#ifdef __DJGPP__
	    /*
	     * (richdawe@bigfoot.com): Old MS-DOS systems may have a maximum
	     * of 8MB memory == XD_PAGE_SIZE * 8. Therefore, do what the user
	     * asks for on MS-DOS.
	     */
#else /* !__DJGPP__ */
	    l = MAX (l, XD_PAGE_SIZE * 8);
#endif /* __DJGPP__ */

	    max_mapped_pages = l / XD_PAGE_SIZE;
	  }
	  break;
	case 'h': help (); break;
	case 'v': version (); break;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  compress_level = c - '0';
	  break;
	case '?':
	default:
	  xd_error ("illegal argument, use --help for help\n");
	  return 2;
	}
    }

  if (verbose && max_mapped_pages < G_MAXINT)
    xd_error ("using %d kilobytes of buffer space\n", (max_mapped_pages * XD_PAGE_SIZE) >> 10);

  argc -= optind;
  argv += optind;

  if (cmd->nargs >= 0 && argc != cmd->nargs)
    {
      xd_error ("wrong number of arguments\n");
      help ();
      return 2;
    }

  return (* cmd->func) (argc, argv);
}

/* Commands */

#define READ_TYPE 1
#define READ_NOSEEK_TYPE 1
#define READ_SEEK_TYPE   3
#define WRITE_TYPE 4

/* Note to the casual reader: the filehandle implemented here is
 * highly aware of the calling patterns of the Xdelta library.  It
 * also plays games with narrowing to implement several files in the
 * same handle.  So, if you try to modify or use this code, BEWARE.
 * See the repository package for a complete implementation.
 * In fact, its really quite a hack. */

static gssize xd_handle_map_page (XdFileHandle *fh, guint pgno, const guint8** mem);
static gboolean xd_handle_unmap_page (XdFileHandle *fh, guint pgno, const guint8** mem);

static gboolean
xd_fwrite (XdFileHandle* fh, const void* buf, gint nbyte)
{
  return fwrite (buf, nbyte, 1, fh->out) == 1;
}

static gboolean
xd_fread (XdFileHandle* fh, void* buf, gint nbyte)
{
  return fread (buf, nbyte, 1, fh->in) == 1;
}

static gboolean
xd_fclose (XdFileHandle* fh)
{
  return fclose (fh->out) == 0;
}

static gboolean
xd_frclose (XdFileHandle* fh)
{
  return fclose (fh->in) == 0;
}

static gboolean
xd_gzwrite (XdFileHandle* fh, const void* buf, gint nbyte)
{
  return gzwrite (fh->out, (void*) buf, nbyte) == nbyte;
}

static gboolean
xd_gzread (XdFileHandle* fh, void* buf, gint nbyte)
{
  return gzread (fh->in, buf, nbyte) == nbyte;
}

static gboolean
xd_gzclose (XdFileHandle* fh)
{
  return gzclose (fh->out) == Z_OK;
}

static gboolean
xd_gzrclose (XdFileHandle* fh)
{
  return gzclose (fh->in) == Z_OK;
}

static void
init_table (XdFileHandle* fh)
{
  fh->lru_table = g_ptr_array_new ();
  fh->lru_chunk = g_mem_chunk_create(LRU, 1<<9, G_ALLOC_ONLY);
  fh->lru_head = NULL;
  fh->lru_tail = NULL;
}

static XdFileHandle*
open_common (const char* name, const char* real_name)
{
  XdFileHandle* fh;

  gint fd;
  struct stat buf;

  if ((fd = open (name, O_RDONLY | O_BINARY, 0)) < 0)
    {
      xd_error ("open %s failed: %s\n", name, g_strerror (errno));
      return NULL;
    }

  if (stat (name, &buf) < 0)
    {
      xd_error ("stat %s failed: %s\n", name, g_strerror (errno));
      return NULL;
    }

  /* S_ISREG() is not on Windows */
  if ((buf.st_mode & S_IFMT) != S_IFREG)
    {
      xd_error ("%s is not a regular file\n", name);
      return NULL;
    }

  fh = g_new0 (XdFileHandle, 1);

  fh->fh.table = & xd_handle_table;
  fh->name = real_name;
  fh->fd = fd;
  fh->length = buf.st_size;
  fh->narrow_high = buf.st_size;

  return fh;
}

static gboolean
file_gzipped (const char* name, gboolean *is_compressed)
{
  FILE* f = fopen (name, FOPEN_READ_ARG);
  guint8 buf[2];

  (*is_compressed) = FALSE;

  if (! f)
    {
      xd_error ("open %s failed: %s\n", name, g_strerror (errno));
      return FALSE;
    }

  if (fread (buf, 2, 1, f) != 1)
    return TRUE;

#define GZIP_MAGIC1 037
#define GZIP_MAGIC2 0213

  if (buf[0] == GZIP_MAGIC1 && buf[1] == GZIP_MAGIC2)
    (* is_compressed) = TRUE;

  return TRUE;
}

static const char*
xd_tmpname (void)
{
  const char* tmpdir = g_get_tmp_dir ();
  GString* s;
  gint x = getpid ();
  static gint seq = 0;
  struct stat buf;

  s = g_string_new (NULL);

  do
    {
      /*
       * (richdawe@bigfoot.com): Limit temporary filenames to
       * the MS-DOS 8+3 convention for DJGPP.
       */
      g_string_sprintf (s, "%s/xd-%05d.%03d", tmpdir, x, seq++);
    }
  while (lstat (s->str, &buf) == 0);

  return s->str;
}

static const char*
file_gunzip (const char* name)
{
  const char* new_name = xd_tmpname ();
  FILE* out = fopen (new_name, FOPEN_WRITE_ARG);
  gzFile in = gzopen (name, "rb");
  guint8 buf[1024];
  int nread, ret;

  while ((nread = gzread (in, buf, 1024)) > 0)
    {
      if (fwrite (buf, nread, 1, out) != 1)
	{
	  xd_error ("write %s failed (during uncompression): %s\n", new_name, g_strerror (errno));
	  return NULL;
	}
    }

  if (nread < 0)
    {
      xd_error ("gzread %s failed\n", name);
      return NULL;
    }

  ret = gzclose (in);
  if (ret != Z_OK)
    {
      xd_error ("gzip input decompression failed: %s\n", name);
      return NULL;
    }

  if (fclose (out))
    {
      xd_error ("close %s failed (during uncompression): %s\n", new_name, g_strerror (errno));
      return NULL;
    }

  return new_name;
}

static XdFileHandle*
open_read_noseek_handle (const char* name, gboolean* is_compressed, gboolean will_read, gboolean honor_pristine)
{
  XdFileHandle* fh;
  const char* name0 = name;

  /* we _could_ stream-read this file if compressed, but it adds a
   * lot of complexity.  the library can handle it, just set the
   * length to (XDELTA_MAX_FILE_LEN-1) and make sure that the end
   * of file condition is set when on the last page.  However, I
   * don't feel like it. */
  if (honor_pristine && pristine)
    *is_compressed = FALSE;
  else
    {
      if (! file_gzipped (name, is_compressed))
        return NULL;
    }

  if ((* is_compressed) && ! (name = file_gunzip (name)))
    return NULL;

  if (! (fh = open_common (name, name0)))
    return NULL;

  fh->type = READ_NOSEEK_TYPE;

  edsio_md5_init (&fh->ctx);

  if (*is_compressed)
    fh->cleanup = name;

  if (will_read)
    {
      g_assert (fh->fd >= 0);
      if (! (fh->in = fdopen (dup (fh->fd), FOPEN_READ_ARG)))
	{
	  xd_error ("fdopen: %s\n", g_strerror (errno));
	  return NULL;
	}
      fh->in_read = &xd_fread;
      fh->in_close = &xd_frclose;
    }
  else
    {
      init_table (fh);
    }

  return fh;
}

static void
xd_read_close (XdFileHandle* fh)
{
  /*
   * (richdawe@bigfoot.com): On Unix you can unlink a file while it is
   * still open. On MS-DOS this can lead to filesystem corruption. Close
   * the file before unlinking it.
   */
  close (fh->fd);

  if (fh->cleanup)
    unlink (fh->cleanup);

  if (fh->in)
    (*fh->in_close) (fh);
}

static XdFileHandle*
open_read_seek_handle (const char* name, gboolean* is_compressed, gboolean honor_pristine)
{
  XdFileHandle* fh;
  const char* name0 = name;

  if (honor_pristine && pristine)
    *is_compressed = FALSE;
  else
    {
      if (! file_gzipped (name, is_compressed))
	return NULL;
    }

  if ((* is_compressed) && ! (name = file_gunzip (name)))
    return NULL;

  if (! (fh = open_common (name, name0)))
    return NULL;

  fh->type = READ_SEEK_TYPE;

  if (*is_compressed)
    fh->cleanup = name;

  init_table (fh);

  edsio_md5_init (&fh->ctx);

  return fh;
}

static XdFileHandle*
open_write_handle (int fd, const char* name)
{
  XdFileHandle* fh = g_new0 (XdFileHandle, 1);
  int nfd;

  fh->fh.table = & xd_handle_table;
  fh->out_fd = fd;
  fh->out_write = &xd_fwrite;
  fh->out_close = &xd_fclose;

  g_assert (fh->out_fd >= 0);

  nfd = dup (fh->out_fd);

  if (! (fh->out = fdopen (nfd, FOPEN_WRITE_ARG)))
    {
      xd_error ("fdopen %s failed: %s\n", name, g_strerror (errno));
      return NULL;
    }

  fh->type = WRITE_TYPE;
  fh->name = name;

  edsio_md5_init (&fh->ctx);

  return fh;
}

static gint
xd_begin_compression (XdFileHandle* fh)
{
  gint filepos, nfd;

  if (compress_level == 0)
    return fh->real_length;

  if (! (fh->out_close) (fh))
    {
      xd_error ("fclose failed: %s\n", g_strerror (errno));
      return -1;
    }

  filepos = lseek (fh->out_fd, 0, SEEK_END);

  if (filepos < 0)
    {
      xd_error ("lseek failed: %s\n", g_strerror (errno));
      return -1;
    }

  g_assert (fh->out_fd >= 0);

  nfd = dup (fh->out_fd);

  fh->out = gzdopen (nfd, "wb");
  fh->out_write = &xd_gzwrite;
  fh->out_close = &xd_gzclose;

  if (! fh->out)
    {
      xd_error ("gzdopen failed: %s\n", g_strerror (errno));
      return -1;
    }

  if (gzsetparams(fh->out, compress_level, Z_DEFAULT_STRATEGY) != Z_OK)
    {
      int foo;
      xd_error ("gzsetparams failed: %s\n", gzerror (fh->out, &foo));
      return -1;
    }

  return filepos;
}

static gboolean
xd_end_compression (XdFileHandle* fh)
{
  if (compress_level == 0)
    return TRUE;

  if (! (fh->out_close) (fh))
    {
      xd_error ("fdclose failed: %s\n", g_strerror (errno));
      return FALSE;
    }

  if (lseek (fh->out_fd, 0, SEEK_END) < 0)
    {
      xd_error ("lseek failed: %s\n", g_strerror (errno));
      return FALSE;
    }

  g_assert (fh->out_fd >= 0);
  fh->out = fdopen (dup (fh->out_fd), FOPEN_WRITE_ARG);
  fh->out_write = &xd_fwrite;
  fh->out_close = &xd_fclose;

  if (! fh->out)
    {
      xd_error ("fdopen failed: %s\n", g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}

static gssize
xd_handle_length (XdFileHandle *fh)
{
  if (fh->in_compressed)
    return fh->current_pos;
  else
    return fh->narrow_high - fh->narrow_low;
}

static gssize
xd_handle_pages (XdFileHandle *fh)
{
  g_assert (fh->type & READ_TYPE);
  return xd_handle_length (fh) / XD_PAGE_SIZE;
}

static gssize
xd_handle_pagesize (XdFileHandle *fh)
{
  g_assert (fh->type & READ_TYPE);
  return XD_PAGE_SIZE;
}

static gint
on_page (XdFileHandle* fh, guint pgno)
{
  if (pgno > xd_handle_pages (fh))
    return -1;

  if (pgno == xd_handle_pages (fh))
    return xd_handle_length (fh) % XD_PAGE_SIZE;

  return XD_PAGE_SIZE;
}

static gboolean
xd_handle_close (XdFileHandle *fh, gint ignore)
{
  /* this is really a reset for writable files */

  if (fh->type == WRITE_TYPE)
    {
      if (fh->reset_length_next_write)
	{
	  fh->reset_length_next_write = FALSE;
	  fh->length = 0;
	  fh->narrow_high = 0;
	}

      fh->reset_length_next_write = TRUE;
      edsio_md5_final (fh->md5, &fh->ctx);
      edsio_md5_init (&fh->ctx);
    }
  else if (fh->in)
    {
      edsio_md5_final (fh->md5, &fh->ctx);
      edsio_md5_init (&fh->ctx);
      fh->md5_good = FALSE;
    }

  return TRUE;
}

static const guint8*
xd_handle_checksum_md5 (XdFileHandle *fh)
{
  if (fh->in && ! fh->md5_good)
    {
      edsio_md5_final (fh->md5, &fh->ctx);
      fh->md5_good = TRUE;
    }
  else if (fh->type != WRITE_TYPE && !fh->in)
    {
      const guint8* page;

      while (fh->md5_page <= xd_handle_pages (fh))
	{
	  gint pgno = fh->md5_page;
	  gint onpage;

	  if ((onpage = xd_handle_map_page (fh, pgno, &page)) < 0)
	    return NULL;

	  if (pgno == fh->md5_page)
	    {
	      fh->md5_page += 1;
	      edsio_md5_update (&fh->ctx, page, onpage);

	      if (fh->md5_page > xd_handle_pages (fh))
		edsio_md5_final (fh->md5, &fh->ctx);
	    }

	  if (! xd_handle_unmap_page (fh, pgno, &page))
	    return NULL;
	}
    }

  return g_memdup (fh->md5, 16);
}

static gboolean
xd_handle_set_pos (XdFileHandle *fh, guint pos)
{
  if (fh->current_pos == pos + fh->narrow_low)
    return TRUE;

  if (pos + fh->narrow_low > fh->narrow_high)
    {
      xd_error ("unexpected EOF in %s\n", fh->name);
      return FALSE;
    }

  fh->current_pos = pos + fh->narrow_low;

  if (fseek (fh->in, fh->current_pos, SEEK_SET))
    {
      xd_error ("fseek failed: %s\n", g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}

static gboolean
xd_handle_narrow (XdFileHandle* fh, guint low, guint high, gboolean compressed)
{
  if (high > fh->length)
    {
      xd_error ("%s: corrupt or truncated delta\n", fh->name);
      return FALSE;
    }

  fh->narrow_low = low;
  fh->narrow_high = high;

  edsio_md5_init (&fh->ctx);

  if (compressed)
    {
      (* fh->in_close) (fh);

      if (lseek (fh->fd, low, SEEK_SET) < 0)
	{
	  xd_error ("%s: corrupt or truncated delta: cannot seek to %d: %s\n", fh->name, low, g_strerror (errno));
	  return FALSE;
	}

      g_assert (fh->fd >= 0);
      fh->in = gzdopen (dup (fh->fd), "rb");
      fh->in_read =  &xd_gzread;
      fh->in_close = &xd_gzrclose;
      fh->in_compressed = TRUE;
      fh->current_pos = 0;

      if (! fh->in)
	{
	  xd_error ("gzdopen failed: %s\n", g_strerror (errno));
	  return -1;
	}
    }
  else
    {
      if (! xd_handle_set_pos (fh, 0))
	return FALSE;
    }

  return TRUE;
}

static guint
xd_handle_get_pos (XdFileHandle* fh)
{
  return fh->current_pos - fh->narrow_low;
}

static const gchar*
xd_handle_name (XdFileHandle *fh)
{
  return g_strdup (fh->name);
}

static gssize
xd_handle_read (XdFileHandle *fh, guint8 *buf, gsize nbyte)
{
  if (nbyte == 0)
    return 0;

  if (! (fh->in_read) (fh, buf, nbyte)) /* This is suspicious */
    {
      xd_error ("read failed: %s\n", g_strerror (errno));
      return -1;
    }

  if (!no_verify)
    edsio_md5_update (&fh->ctx, buf, nbyte);

  fh->current_pos += nbyte;

  return nbyte;
}

static gboolean
xd_handle_write (XdFileHandle *fh, const guint8 *buf, gsize nbyte)
{
  g_assert (fh->type == WRITE_TYPE);

  if (fh->reset_length_next_write)
    {
      fh->reset_length_next_write = FALSE;
      fh->length = 0;
      fh->narrow_high = 0;
    }

  if (! no_verify)
    edsio_md5_update (&fh->ctx, buf, nbyte);

  if (! (*fh->out_write) (fh, buf, nbyte))
    {
      xd_error ("write failed: %s\n", g_strerror (errno));
      return FALSE;
    }

  fh->length += nbyte;
  fh->real_length += nbyte;
  fh->narrow_high += nbyte;

  return TRUE;
}

static gboolean
xd_handle_really_close (XdFileHandle *fh)
{
  g_assert (fh->type == WRITE_TYPE);

  if (! (* fh->out_close) (fh) || close (fh->out_fd) < 0)
    {
      xd_error ("write failed: %s\n", g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}

static LRU*
pull_lru (XdFileHandle* fh, LRU* lru)
{
  if (lru->next && lru->prev)
    {
      lru->next->prev = lru->prev;
      lru->prev->next = lru->next;
    }
  else if (lru->next)
    {
      fh->lru_tail = lru->next;
      lru->next->prev = NULL;
    }
  else if (lru->prev)
    {
      fh->lru_head = lru->prev;
      lru->prev->next = NULL;
    }
  else
    {
      fh->lru_head = NULL;
      fh->lru_tail = NULL;
    }

  lru->next = NULL;
  lru->prev = NULL;

  return lru;
}

static gboolean
really_free_one_page (XdFileHandle* fh)
{
  LRU *lru = fh->lru_tail;

  for (; lru; lru = lru->prev)
    {
      gint to_unmap;
      LRU *lru_dead;

      if (lru->refs > 0)
	continue;

      lru_dead = pull_lru (fh, lru);

      g_assert (lru_dead->buffer);

      to_unmap = on_page (fh, lru_dead->page);

      fh->lru_count -= 1;

      if (to_unmap > 0)
	{
#ifdef WINHACK
	  g_free (lru_dead->buffer);
#else
	  if (munmap (lru_dead->buffer, to_unmap))
	    {
	      xd_error ("munmap failed: %s\n", g_strerror (errno));
	      return FALSE;
	    }
#endif /* WINHACK */
	}

      lru_dead->buffer = NULL;

      return TRUE;
    }

  return TRUE;
}

#if 0
static void
print_lru (XdFileHandle* fh)
{
  LRU* lru = fh->lru_head;

  for (; lru; lru = lru->prev)
    {
      g_print ("page %d buffer %p\n", lru->page, lru->buffer);

      if (! lru->prev && lru != fh->lru_tail)
	g_print ("incorrect lru_tail\n");
    }
}
#endif

static gboolean
make_lru_room (XdFileHandle* fh)
{
  if (fh->lru_count == max_mapped_pages)
    {
      if (! really_free_one_page (fh))
	return FALSE;
    }

  g_assert (fh->lru_count < max_mapped_pages);

  return TRUE;
}

/*#define DEBUG_MAP*/

static gssize
xd_handle_map_page (XdFileHandle *fh, guint pgno, const guint8** mem)
{
  LRU* lru;
  guint to_map;

#ifdef DEBUG_MAP
  g_print ("map %p:%d\n", fh, pgno);
#endif

  g_assert (fh->type & READ_TYPE);

  if (fh->lru_table->len < (pgno + 1))
    {
      gint olen = fh->lru_table->len;

      g_ptr_array_set_size (fh->lru_table, pgno + 1);

      while (olen <= pgno)
	fh->lru_table->pdata[olen++] = NULL;
    }

  lru = fh->lru_table->pdata[pgno];

  if (! lru)
    {
      lru = g_chunk_new0 (LRU, fh->lru_chunk);
      fh->lru_table->pdata[pgno] = lru;
      lru->page = pgno;
    }
  else if (lru->buffer)
    {
      pull_lru (fh, lru);
    }

  lru->prev = fh->lru_head;
  lru->next = NULL;

  fh->lru_head = lru;

  if (lru->prev)
    lru->prev->next = lru;

  if (! fh->lru_tail)
    fh->lru_tail = lru;

  to_map = on_page (fh, pgno);

  if (to_map < 0)
    {
      xd_error ("unexpected EOF in %s\n", fh->name);
      return -1;
    }

  if (! lru->buffer)
    {
      if (! make_lru_room (fh))
	return -1;

      fh->lru_count += 1;

      if (to_map > 0)
	{
#ifdef WINHACK
	  lru->buffer = g_malloc (to_map);

	  if (lseek (fh->fd, pgno * XD_PAGE_SIZE, SEEK_SET) < 0)
	    {
	      xd_error ("lseek failed: %s\n", g_strerror (errno));
	      return -1;
	    }

	  if (read (fh->fd, lru->buffer, to_map) != to_map)
	    {
	      xd_error ("read failed: %s\n", g_strerror (errno));
	      return -1;
	    }
#else
	  if (! (lru->buffer = mmap (NULL, to_map, PROT_READ, MAP_PRIVATE, fh->fd, pgno * XD_PAGE_SIZE)))
	    {
	      xd_error ("mmap failed: %s\n", g_strerror (errno));
	      return -1;
	    }
#endif
	}
      else
	{
	  lru->buffer = (void*) -1;
	}

      if (pgno == fh->md5_page)
	{
	  if (! no_verify)
	    edsio_md5_update (&fh->ctx, lru->buffer, to_map);
	  fh->md5_page += 1;

	  if (fh->md5_page > xd_handle_pages (fh))
	    edsio_md5_final (fh->md5, &fh->ctx);
	}
    }

  (*mem) = lru->buffer;

  lru->refs += 1;
  fh->lru_outstanding_refs += 1;

  return to_map;
}

static gboolean
xd_handle_unmap_page (XdFileHandle *fh, guint pgno, const guint8** mem)
{
  LRU* lru;

#ifdef DEBUG_MAP
  g_print ("unmap %p:%d\n", fh, pgno);
#endif

  g_assert (fh->type & READ_TYPE);

  g_assert (pgno < fh->lru_table->len);

  lru = fh->lru_table->pdata[pgno];

  g_assert (lru && lru->refs > 0);

  g_assert (lru->buffer == (*mem));

  (*mem) = NULL;

  lru->refs -= 1;
  fh->lru_outstanding_refs += 1;

  if (lru->refs == 0 && fh->type == READ_NOSEEK_TYPE)
    {
      pull_lru (fh, lru);

      lru->next = fh->lru_tail;
      if (lru->next) lru->next->prev = lru;
      lru->prev = NULL;
      fh->lru_tail = lru;

      if (! really_free_one_page (fh))
	return FALSE;
    }

  return TRUE;
}

static gboolean
xd_handle_copy (XdFileHandle *from, XdFileHandle *to, guint off, guint len)
{
  if (from->in)
    {
      guint8 buf[1024];

      /*if (! xd_handle_set_pos (from, off))
	return FALSE;*/

      while (len > 0)
	{
	  guint r = MIN (1024, len);

	  if (xd_handle_read (from, buf, r) != r)
	    return FALSE;

	  if (! xd_handle_write (to, buf, r))
	    return FALSE;

	  len -= r;
	}
    }
  else
    {
      while (len > 0)
	{
	  guint off_page = off / XD_PAGE_SIZE;
	  guint off_off = off % XD_PAGE_SIZE;

	  gint on = on_page (from, off_page);
	  guint rem;
	  guint copy;

	  if (on <= 0)
	    {
	      xd_error ("unexpected EOF in %s\n", from->name);
	      return FALSE;
	    }

	  rem = on - off_off;
	  copy = MIN (len, rem);

	  if (from->copy_pgno != off_page &&
	      from->copy_page &&
	      ! xd_handle_unmap_page (from, from->copy_pgno, &from->copy_page))
	    return FALSE;

	  from->copy_pgno = off_page;

	  if (xd_handle_map_page (from, off_page, &from->copy_page) < 0)
	    return FALSE;

	  if (! xd_handle_write (to, from->copy_page + off_off, copy))
	    return FALSE;

	  if (! xd_handle_unmap_page (from, off_page, &from->copy_page))
	    return FALSE;

	  len -= copy;
	  off += copy;
	}
    }

  return TRUE;
}

static gboolean
xd_handle_putui (XdFileHandle *fh, guint32 i)
{
  guint32 hi = g_htonl (i);

  return xd_handle_write (fh, (guint8*)&hi, 4);
}

static gboolean
xd_handle_getui (XdFileHandle *fh, guint32* i)
{
  if (xd_handle_read (fh, (guint8*)i, 4) != 4)
    return FALSE;

  *i = g_ntohl (*i);

  return TRUE;
}

static HandleFuncTable xd_handle_table =
{
  (gssize (*) (FileHandle *fh)) xd_handle_length,
  (gssize (*) (FileHandle *fh)) xd_handle_pages,
  (gssize (*) (FileHandle *fh)) xd_handle_pagesize,
  (gssize (*) (FileHandle *fh, guint pgno, const guint8** mem)) xd_handle_map_page,
  (gboolean (*) (FileHandle *fh, guint pgno, const guint8** mem)) xd_handle_unmap_page,
  (const guint8* (*) (FileHandle *fh)) xd_handle_checksum_md5,

  (gboolean (*) (FileHandle *fh, gint flags)) xd_handle_close,

  (gboolean (*) (FileHandle *fh, const guint8 *buf, gsize nbyte)) xd_handle_write,
  (gboolean (*) (FileHandle *from, FileHandle *to, guint off, guint len)) xd_handle_copy,

  (gboolean (*) (FileHandle *fh, guint32* i)) xd_handle_getui,
  (gboolean (*) (FileHandle *fh, guint32 i)) xd_handle_putui,
  (gssize   (*) (FileHandle *fh, guint8 *buf, gsize nbyte)) xd_handle_read,
  (const gchar* (*) (FileHandle *fh)) xd_handle_name,
};

static void
htonl_array (guint32* array, gint len)
{
  gint i;

  for (i = 0; i < len; i += 1)
    array[i] = g_htonl(array[i]);
}

static void
ntohl_array (guint32* array, gint len)
{
  gint i;

  for (i = 0; i < len; i += 1)
    array[i] = g_ntohl(array[i]);
}

static gint
delta_command (gint argc, gchar** argv)
{
  gint patch_out_fd;
  const char* patch_out_name;
  XdFileHandle *from, *to, *out;
  XdeltaGenerator* gen;
  XdeltaSource* src;
  XdeltaControl* cont;
  gboolean from_is_compressed = FALSE, to_is_compressed = FALSE;
  guint32 control_offset, header_offset;
  const char* from_name, *to_name;
  guint32 header_space[HEADER_WORDS];
  int fd;

  memset (header_space, 0, sizeof (header_space));

  if (argc != 3)
    {
      xd_error ("usage: %s delta fromfile tofile patchfile\n", program_name);
      return 2;
    }

  if (verbose)
    {
      xd_error ("using block size: %d bytes\n", xdp_blocksize ());
    }

  if (! (from = open_read_seek_handle (argv[0], &from_is_compressed, TRUE)))
    return 2;

  if (! (to = open_read_noseek_handle (argv[1], &to_is_compressed, FALSE, TRUE)))
    return 2;

  // Note: I tried support to patches to stdout, but it broke when
  // compression was added.  Sigh
  fd = open (argv[2], O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);

  if (fd < 0)
    {
      xd_error ("open %s failed: %s\n", argv[2], g_strerror (errno));
      return 2;
    }

  patch_out_fd = fd;
  patch_out_name = argv[2];

  from_name = g_basename (argv[0]);
  to_name = g_basename (argv[1]);

  if (! (out = open_write_handle (patch_out_fd, patch_out_name)))
    return 2;

  if (! (gen = xdp_generator_new ()))
    return 2;

  if (! (src = xdp_source_new (from_name, (FileHandle*) from, NULL, NULL)))
    return 2;

  xdp_source_add (gen, src);

  if (! xd_handle_write (out, XDELTA_PREFIX, XDELTA_PREFIX_LEN))
    return 2;

  /* compute the header */
  header_space[0] = 0;

  if (no_verify) header_space[0]           |= FLAG_NO_VERIFY;
  if (from_is_compressed) header_space[0]  |= FLAG_FROM_COMPRESSED;
  if (to_is_compressed) header_space[0]    |= FLAG_TO_COMPRESSED;
  if (compress_level != 0) header_space[0] |= FLAG_PATCH_COMPRESSED;

  header_space[1] = strlen (from_name) << 16 | strlen (to_name);
  /* end compute the header */

  htonl_array (header_space, HEADER_WORDS);

  if (! xd_handle_write (out, (guint8*) header_space, HEADER_SPACE))
    return 2;

  if (! xd_handle_write (out, from_name, strlen (from_name)))
    return 2;

  if (! xd_handle_write (out, to_name, strlen (to_name)))
    return 2;

  if (! xd_handle_close (out, 0))
    return 2;

  if ((header_offset = xd_begin_compression (out)) < 0)
    return 2;

  if (! (cont = xdp_generate_delta (gen, (FileHandle*) to, NULL, (FileHandle*) out)))
    return 2;

#if 0
  {
    guint32 pos = 0;
    gint i, l = cont->inst_len;

    for (i = 0; i < l; i += 1)
      {
	XdeltaInstruction *inst = cont->inst + i;
	if (inst->index == 0) {
	  inst->index = 999999999;
	  inst->offset = 999999999;
	} else {
	  inst->index = pos;
	}
	pos += inst->length;
      }
  }
  serializeio_print_xdeltacontrol_obj (cont, 0);
#endif

  if (cont->has_data && cont->has_data == cont->source_info_len)
    {
      if (! quiet)
	xd_error ("warning: no matches found in from file, patch will apply without it\n");
    }

  if (! xd_handle_close (out, 0))
    return 2;

  if ((control_offset = xd_begin_compression (out)) < 0)
    return 2;

  if (! xdp_control_write (cont, (FileHandle*) out))
    return 2;

  if (! xd_end_compression (out))
    return 2;

  if (! xd_handle_putui (out, control_offset))
    return 2;

  if (! xd_handle_write (out, XDELTA_PREFIX, XDELTA_PREFIX_LEN))
    return 2;

  xd_read_close (from);
  xd_read_close (to);

  if (! xd_handle_really_close (out))
    return 2;

  xdp_generator_free (gen);

  /* Note: prior to 1.1.5:
   * return control_offset != header_offset;
   */
  return 0;
}

static XdeltaPatch*
process_patch (const char* name)
{
  XdeltaPatch* patch;
  guint total_trailer;

  patch = g_new0 (XdeltaPatch, 1);

  patch->patch_name = name;

  /* Strictly speaking, I'm violating the intended semantics of noseek here.
   * It will seek the file, which is not in fact checked in the map/unmap
   * logic above.  This only means that it will not cache pages of this file
   * since it will be read piecewise sequentially. */
  if (! (patch->patch_in = open_read_noseek_handle (name, &patch->patch_is_compressed, TRUE, TRUE)))
    return NULL;

  if (xd_handle_read (patch->patch_in, patch->magic_buf, XDELTA_PREFIX_LEN) != XDELTA_PREFIX_LEN)
    return NULL;

  if (xd_handle_read (patch->patch_in, (guint8*) patch->header_space, HEADER_SPACE) != HEADER_SPACE)
    return NULL;

  ntohl_array (patch->header_space, HEADER_WORDS);

  if (strncmp (patch->magic_buf, XDELTA_110_PREFIX, XDELTA_PREFIX_LEN) == 0)
    {
      patch->has_trailer = TRUE;
      patch->patch_version = "1.1";
    }
  else if (strncmp (patch->magic_buf, XDELTA_104_PREFIX, XDELTA_PREFIX_LEN) == 0)
    {
      patch->has_trailer = TRUE;
      patch->patch_version = "1.0.4";
    }
  else if (strncmp (patch->magic_buf, XDELTA_100_PREFIX, XDELTA_PREFIX_LEN) == 0)
    {
      patch->patch_version = "1.0";
    }
  else if (strncmp (patch->magic_buf, XDELTA_020_PREFIX, XDELTA_PREFIX_LEN) == 0)
    goto nosupport;
  else if (strncmp (patch->magic_buf, XDELTA_018_PREFIX, XDELTA_PREFIX_LEN) == 0)
    goto nosupport;
  else if (strncmp (patch->magic_buf, XDELTA_014_PREFIX, XDELTA_PREFIX_LEN) == 0)
    goto nosupport;
  else
    {
      xd_error ("%s: bad magic number: not a valid delta\n", name);
      return NULL;
    }

  patch->patch_flags = patch->header_space[0];

  if (no_verify)
    xd_error ("--noverify is only accepted when creating a delta\n");

  if (patch->patch_flags & FLAG_NO_VERIFY)
    no_verify = TRUE;
  else
    no_verify = FALSE;

  patch->from_name_len = patch->header_space[1] >> 16;
  patch->to_name_len = patch->header_space[1] & 0xffff;

  patch->from_name = g_malloc (patch->from_name_len+1);
  patch->to_name = g_malloc (patch->to_name_len+1);

  patch->from_name[patch->from_name_len] = 0;
  patch->to_name[patch->to_name_len] = 0;

  if (xd_handle_read (patch->patch_in, patch->from_name, patch->from_name_len) != patch->from_name_len)
    return NULL;

  if (xd_handle_read (patch->patch_in, patch->to_name, patch->to_name_len) != patch->to_name_len)
    return NULL;

  patch->header_offset = xd_handle_get_pos (patch->patch_in);

  total_trailer = 4 + (patch->has_trailer ? XDELTA_PREFIX_LEN : 0);

  if (! xd_handle_set_pos (patch->patch_in, xd_handle_length (patch->patch_in) - total_trailer))
    return NULL;

  if (! xd_handle_getui (patch->patch_in, &patch->control_offset))
    return NULL;

  if (patch->has_trailer)
    {
      guint8 trailer_buf[XDELTA_PREFIX_LEN];

      if (xd_handle_read (patch->patch_in, trailer_buf, XDELTA_PREFIX_LEN) != XDELTA_PREFIX_LEN)
	return NULL;

      if (strncmp (trailer_buf, patch->magic_buf, XDELTA_PREFIX_LEN) != 0)
	{
	  xd_error ("%s: bad trailing magic number, delta is corrupt\n", name);
	  return NULL;
	}
    }

  if (! xd_handle_narrow (patch->patch_in, patch->control_offset,
			  xd_handle_length (patch->patch_in) - total_trailer,
			  patch->patch_flags & FLAG_PATCH_COMPRESSED))
    return NULL;

  if (! (patch->cont = xdp_control_read ((FileHandle*) patch->patch_in)))
    return NULL;

  if (patch->cont->source_info_len > 0)
    {
      XdeltaSourceInfo* info = patch->cont->source_info[0];

      if (info->isdata)
	patch->data_source = info;
      else
	{
	  patch->from_source = info;

	  if (patch->cont->source_info_len > 1)
	    {
	      xd_generate_void_event (EC_XdIncompatibleDelta);
	      return NULL;
	    }
	}
    }

  if (patch->cont->source_info_len > 1)
    {
      patch->from_source = patch->cont->source_info[1];
    }

  if (patch->cont->source_info_len > 2)
    {
      xd_generate_void_event (EC_XdIncompatibleDelta);
      return NULL;
    }

  if (! xd_handle_narrow (patch->patch_in,
			  patch->header_offset,
			  patch->control_offset,
			  patch->patch_flags & FLAG_PATCH_COMPRESSED))
    return NULL;

  return patch;

 nosupport:

  xd_error ("delta format is unsupported (too old)\n");
  return NULL;
}

static gint
info_command (gint argc, gchar** argv)
{
  XdeltaPatch* patch;
  char buf[33];
  int i;
  XdeltaSourceInfo* si;

  if (! (patch = process_patch (argv[0])))
    return 2;

  xd_error_file = stdout;

  xd_error ("version %s found patch version %s in %s%s\n",
	    xdelta_version,
	    patch->patch_version,
	    patch->patch_name,
	    patch->patch_flags & FLAG_PATCH_COMPRESSED ? " (compressed)" : "");

  if (patch->patch_flags & FLAG_NO_VERIFY)
    xd_error ("generated with --noverify\n");

  if (patch->patch_flags & FLAG_FROM_COMPRESSED)
    xd_error ("generated with a gzipped FROM file\n");

  if (patch->patch_flags & FLAG_TO_COMPRESSED)
    xd_error ("generated with a gzipped TO file\n");

  edsio_md5_to_string (patch->cont->to_md5, buf);

  xd_error ("output name:   %s\n", patch->to_name);
  xd_error ("output length: %d\n", patch->cont->to_len);
  xd_error ("output md5:    %s\n", buf);

  xd_error ("patch from segments: %d\n", patch->cont->source_info_len);

  xd_error ("MD5\t\t\t\t\tLength\tCopies\tUsed\tSeq?\tName\n");

  for (i = 0; i < patch->cont->source_info_len; i += 1)
    {
      si = patch->cont->source_info[i];

      edsio_md5_to_string (si->md5, buf);

      xd_error ("%s\t%d\t%d\t%d\t%s\t%s\n",
		buf,
		si->len,
		si->copies,
		si->copy_length,
		si->sequential ? "yes" : "no",
		si->name);
    }

  return 0;
}

static gint
patch_command (gint argc, gchar** argv)
{
  XdFileHandle* to_out;
  XdeltaPatch* patch;
  gint to_out_fd;
  int count = 1;
  int ret;

  if (argc < 1 || argc > 3)
    {
      xd_error ("usage: %s patch patchfile [fromfile [tofile]]\n", program_name);
      return 2;
    }

  if (! (patch = process_patch (argv[0])))
    return 2;

  if (argc > 1)
    patch->from_name = argv[1];
  else if (verbose)
    xd_error ("using from file name: %s\n", patch->from_name);

  if (argc > 2)
    patch->to_name = argv[2];
  else
    {
      struct stat sbuf;
      gchar *defname = g_strdup (patch->to_name);

      while ((ret = stat (patch->to_name, & sbuf)) == 0)
	{
	  if (verbose)
	    xd_error ("to file exists: %s\n", patch->to_name);
	  patch->to_name = g_strdup_printf ("%s.xdp%d", defname, count++);
	}

      if (verbose || strcmp (defname, patch->to_name) != 0)
	xd_error ("using to file name: %s\n", patch->to_name);
    }

  if (strcmp (patch->to_name, "-") == 0)
    {
      to_out_fd = STDOUT_FILENO;
      patch->to_name = "standard output";
    }
  else
    {
      to_out_fd = open (patch->to_name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);

      if (to_out_fd < 0)
	{
	  xd_error ("open %s failed: %s\n", patch->to_name, g_strerror (errno));
	  return 2;
	}
    }

  to_out = open_write_handle (to_out_fd, patch->to_name);

  if ((patch->patch_flags & FLAG_TO_COMPRESSED) && (xd_begin_compression (to_out) < 0))
    return 2;

  if (patch->from_source)
    {
      XdFileHandle* from_in;
      gboolean from_is_compressed = FALSE;

      if (! (from_in = open_read_seek_handle (patch->from_name, &from_is_compressed, TRUE)))
	return 2;

      if (from_is_compressed != ((patch->patch_flags & FLAG_FROM_COMPRESSED) && 1))
	xd_error ("warning: expected %scompressed from file (%s)\n",
		  (patch->patch_flags & FLAG_FROM_COMPRESSED) ? "" : "un",
		  patch->from_name);

      if (xd_handle_length (from_in) != patch->from_source->len)
	{
	  xd_error ("expected from file (%s) of %slength %d bytes\n",
		    patch->from_name,
		    from_is_compressed ? "uncompressed " : "",
		    patch->from_source->len);
	  return 2;
	}

      patch->from_source->in = (XdeltaStream*) from_in;
    }

  if (patch->data_source)
    patch->data_source->in = (XdeltaStream*) patch->patch_in;

  if (! xdp_apply_delta (patch->cont, (FileHandle*) to_out))
    return 2;

  if (patch->from_source)
    xd_read_close ((XdFileHandle*) patch->from_source->in);

  xd_read_close (patch->patch_in);

  if (! xd_handle_really_close (to_out))
    return 2;

  return 0;
}
