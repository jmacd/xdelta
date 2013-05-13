/* xdelta3 - delta compression tools and library
 * Copyright (C) 2011, 2012 Joshua P. MacDonald
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
#ifndef XDELTA3_INTERNAL_H__
#define XDELTA3_INTERNAL_H__

#include "xdelta3.h"

typedef struct _main_file        main_file;
typedef struct _main_extcomp     main_extcomp;

void main_buffree (void *ptr);
void* main_bufalloc (size_t size);
void main_file_init (main_file *xfile);
int main_file_close (main_file *xfile);
void main_file_cleanup (main_file *xfile);
int main_file_isopen (main_file *xfile);
int main_file_open (main_file *xfile, const char* name, int mode);
int main_file_exists (main_file *xfile);
int xd3_whole_append_window (xd3_stream *stream);
int xd3_main_cmdline (int argc, char **argv);
int main_file_read (main_file  *ifile,
		    uint8_t    *buf,
		    size_t     size,
		    size_t    *nread,
		    const char *msg);
int main_file_write (main_file *ofile, uint8_t *buf, 
		     usize_t size, const char *msg);
int test_compare_files (const char* f0, const char* f1);
usize_t xd3_bytes_on_srcblk (xd3_source *src, xoff_t blkno);
xoff_t xd3_source_eof(const xd3_source *src);
uint32_t xd3_large_cksum_update (uint32_t cksum,
				 const uint8_t *base,
				 usize_t look);
int xd3_encode_init_full (xd3_stream *stream);
#if PYTHON_MODULE || SWIG_MODULE || NOT_MAIN
int xd3_main_cmdline (int argc, char **argv);
#endif

/* main_file->mode values */
typedef enum
{
  XO_READ  = 0,
  XO_WRITE = 1
} main_file_modes;

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
  int                 size_known;    /* Set by main_set_souze */
  xoff_t              source_position;  /* for avoiding seek in getblk_func */
  int                 seek_failed;   /* after seek fails once, try FIFO */
};

/* According to the internet, Windows vsnprintf() differs from most
 * Unix implementations regarding the terminating 0 when the boundary
 * condition is met. It doesn't matter here, we don't rely on the
 * trailing 0.  Besides, both Windows and DJGPP vsnprintf return -1
 * upon truncation, which isn't C99 compliant. To overcome this,
 * recent MinGW runtimes provided their own vsnprintf (notice the
 * absence of the '_' prefix) but they were initially buggy.  So,
 * always use the native '_'-prefixed version with Win32. */
#ifdef _WIN32
#define vsnprintf_func(str,size,fmt,args) \
  _vsnprintf_s(str,size,size-1,fmt,args)
#define snprintf_func(str,size,fmt,...) \
  _snprintf_s(str,size,size-1,fmt,__VA_ARGS__)
#else
#define vsnprintf_func vsnprintf
#define snprintf_func snprintf
#endif
#define short_sprintf(sb,fmt,...) \
  snprintf_func((sb).buf,sizeof((sb).buf),fmt,__VA_ARGS__)

/* Type used for short snprintf calls. */
typedef struct {
  char buf[48];
} shortbuf;

/* Prior to SVN 303 this function was only defined in DJGPP and WIN32
 * environments and other platforms would use the builtin snprintf()
 * with an arrangement of macros below.  In OS X 10.6, Apply made
 * snprintf() a macro, which defeated those macros (since snprintf
 * would be evaluated before its argument macros were expanded,
 * therefore always define xsnprintf_func. */
#undef PRINTF_ATTRIBUTE
#ifdef __GNUC__
/* Let's just assume no one uses gcc 2.x! */
#define PRINTF_ATTRIBUTE(x,y) __attribute__ ((__format__ (__printf__, x, y)))
#else
#define PRINTF_ATTRIBUTE(x,y)
#endif

/* Underlying xprintf() */
int xsnprintf_func (char *str, int n, const char *fmt, ...)
  PRINTF_ATTRIBUTE(3,4);

/* XPR(NT "", ...) (used by main) prefixes an "xdelta3: " to the output. */
void xprintf(const char *fmt, ...) PRINTF_ATTRIBUTE(1,2);
#define XPR xprintf
#define NT "xdelta3: "
#define NTR ""

#ifndef UINT32_MAX
#define UINT32_MAX 4294967295U
#endif

#ifndef UINT64_MAX
#define UINT64_MAX 18446744073709551615ULL
#endif

#endif // XDELTA3_INTERNAL_H__
