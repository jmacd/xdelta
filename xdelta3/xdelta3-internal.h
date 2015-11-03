/* xdelta3 - delta compression tools and library
 * Copyright (C) 2011, 2012, 2013, 2014, 2015 Joshua P. MacDonald
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
int xd3_emit_byte (xd3_stream  *stream,
		   xd3_output **outputp,
		   uint8_t      code);

int xd3_emit_bytes (xd3_stream     *stream,
		    xd3_output    **outputp,
		    const uint8_t  *base,
		    usize_t          size);
xd3_output* xd3_alloc_output (xd3_stream *stream,
			      xd3_output *old_output);

int xd3_encode_init_full (xd3_stream *stream);
size_t xd3_pow2_roundup (size_t x);
int xd3_process_stream (int            is_encode,
			xd3_stream    *stream,
			int          (*func) (xd3_stream *),
			int            close_stream,
			const uint8_t *input,
			usize_t        input_size,
			uint8_t       *output,
			usize_t       *output_size,
			usize_t        output_size_max);

#if PYTHON_MODULE || SWIG_MODULE || NOT_MAIN
int xd3_main_cmdline (int argc, char **argv);
#endif

/* main_file->mode values */
typedef enum
{
  XO_READ  = 0,
  XO_WRITE = 1
} main_file_modes;

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

/* If none are set, default to posix. */
#if (XD3_POSIX + XD3_STDIO + XD3_WIN32) == 0
#undef XD3_POSIX
#define XD3_POSIX 1
#endif

struct _main_file
{
#if XD3_WIN32
  HANDLE              file;
#elif XD3_STDIO
  FILE               *file;
#elif XD3_POSIX
  int                 file;
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

#ifdef _WIN32
#define vsnprintf_func _vsnprintf
#define snprintf_func _snprintf
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

#define UINT32_OFLOW_MASK 0xfe000000U
#define UINT64_OFLOW_MASK 0xfe00000000000000ULL

/*********************************************************************
 Integer encoder/decoder functions
 **********************************************************************/

/* Consume N bytes of input, only used by the decoder. */
#define DECODE_INPUT(n)             \
  do {                              \
  stream->total_in += (xoff_t) (n); \
  stream->avail_in -= (n);          \
  stream->next_in  += (n);          \
  } while (0)

#define DECODE_INTEGER_TYPE(PART,OFLOW)                                \
  while (stream->avail_in != 0)                                        \
    {                                                                  \
      usize_t next = stream->next_in[0];                               \
                                                                       \
      DECODE_INPUT(1);                                                 \
                                                                       \
      if (PART & OFLOW)                                                \
	{                                                              \
	  stream->msg = "overflow in decode_integer";                  \
	  return XD3_INVALID_INPUT;                                    \
	}                                                              \
                                                                       \
      PART = (PART << 7) | (next & 127);                               \
                                                                       \
      if ((next & 128) == 0)                                           \
	{                                                              \
	  (*val) = PART;                                               \
	  PART = 0;                                                    \
	  return 0;                                                    \
	}                                                              \
    }                                                                  \
                                                                       \
  stream->msg = "further input required";                              \
  return XD3_INPUT

#define READ_INTEGER_TYPE(TYPE, OFLOW)                                 \
  TYPE val = 0;                                                        \
  const uint8_t *inp = (*inpp);                                        \
  usize_t next;                                                        \
                                                                       \
  do                                                                   \
    {                                                                  \
      if (inp == maxp)						       \
	{                                                              \
	  stream->msg = "end-of-input in read_integer";                \
	  return XD3_INVALID_INPUT;                                    \
	}                                                              \
                                                                       \
      if (val & OFLOW)                                                 \
	{                                                              \
	  stream->msg = "overflow in read_intger";                     \
	  return XD3_INVALID_INPUT;                                    \
	}                                                              \
                                                                       \
      next = (*inp++);                                                 \
      val  = (val << 7) | (next & 127);                                \
    }                                                                  \
  while (next & 128);                                                  \
                                                                       \
  (*valp) = val;                                                       \
  (*inpp) = inp;                                                       \
                                                                       \
  return 0

#define EMIT_INTEGER_TYPE()                                            \
  /* max 64-bit value in base-7 encoding is 9.1 bytes */               \
  uint8_t buf[10];                                                     \
  usize_t  bufi = 10;                                                  \
                                                                       \
  /* This loop performs division and turns on all MSBs. */             \
  do                                                                   \
    {                                                                  \
      buf[--bufi] = (num & 127) | 128;                                 \
      num >>= 7U;                                                      \
    }                                                                  \
  while (num != 0);                                                    \
                                                                       \
  /* Turn off MSB of the last byte. */                                 \
  buf[9] &= 127;                                                       \
                                                                       \
  return xd3_emit_bytes (stream, output, buf + bufi, 10 - bufi)

#define IF_SIZEOF32(x) if (num < (1U   << (7 * (x)))) return (x);
#define IF_SIZEOF64(x) if (num < (1ULL << (7 * (x)))) return (x);

#if USE_UINT32
static inline uint32_t
xd3_sizeof_uint32_t (uint32_t num)
{
  IF_SIZEOF32(1);
  IF_SIZEOF32(2);
  IF_SIZEOF32(3);
  IF_SIZEOF32(4);
  return 5;
}

static inline int
xd3_decode_uint32_t (xd3_stream *stream, uint32_t *val)
{ DECODE_INTEGER_TYPE (stream->dec_32part, UINT32_OFLOW_MASK); }

static inline int
xd3_read_uint32_t (xd3_stream *stream, const uint8_t **inpp,
		   const uint8_t *maxp, uint32_t *valp)
{ READ_INTEGER_TYPE (uint32_t, UINT32_OFLOW_MASK); }

#if XD3_ENCODER
static inline int
xd3_emit_uint32_t (xd3_stream *stream, xd3_output **output, uint32_t num)
{ EMIT_INTEGER_TYPE (); }
#endif
#endif

#if USE_UINT64
static inline int
xd3_decode_uint64_t (xd3_stream *stream, uint64_t *val)
{ DECODE_INTEGER_TYPE (stream->dec_64part, UINT64_OFLOW_MASK); }

#if XD3_ENCODER
static inline int
xd3_emit_uint64_t (xd3_stream *stream, xd3_output **output, uint64_t num)
{ EMIT_INTEGER_TYPE (); }
#endif

/* These are tested but not used */
#if REGRESSION_TEST
static int
xd3_read_uint64_t (xd3_stream *stream, const uint8_t **inpp,
		   const uint8_t *maxp, uint64_t *valp)
{ READ_INTEGER_TYPE (uint64_t, UINT64_OFLOW_MASK); }

static uint32_t
xd3_sizeof_uint64_t (uint64_t num)
{
  IF_SIZEOF64(1);
  IF_SIZEOF64(2);
  IF_SIZEOF64(3);
  IF_SIZEOF64(4);
  IF_SIZEOF64(5);
  IF_SIZEOF64(6);
  IF_SIZEOF64(7);
  IF_SIZEOF64(8);
  IF_SIZEOF64(9);

  return 10;
}
#endif

#endif

#if SIZEOF_USIZE_T == 4
#define USIZE_T_MAX        UINT32_MAX
#define USIZE_T_MAXBLKSZ   0x80000000U
#define xd3_decode_size   xd3_decode_uint32_t
#define xd3_emit_size     xd3_emit_uint32_t
#define xd3_sizeof_size   xd3_sizeof_uint32_t
#define xd3_read_size     xd3_read_uint32_t
#elif SIZEOF_USIZE_T == 8
#define USIZE_T_MAX        UINT64_MAX
#define USIZE_T_MAXBLKSZ   0x8000000000000000ULL
#define xd3_decode_size   xd3_decode_uint64_t
#define xd3_emit_size     xd3_emit_uint64_t
#define xd3_sizeof_size   xd3_sizeof_uint64_t
#define xd3_read_size     xd3_read_uint64_t
#endif

#if SIZEOF_XOFF_T == 4
#define XOFF_T_MAX        UINT32_MAX
#define xd3_emit_offset   xd3_emit_uint32_t
static inline int
xd3_decode_offset (xd3_stream *stream, xoff_t *val)
{
  return xd3_decode_uint32_t (stream, (uint32_t*) val);
}
#elif SIZEOF_XOFF_T == 8
#define XOFF_T_MAX        UINT64_MAX
#define xd3_emit_offset   xd3_emit_uint64_t
static inline int
xd3_decode_offset (xd3_stream *stream, xoff_t *val)
{
  return xd3_decode_uint64_t (stream, (uint64_t*) val);
}
#endif

#define USIZE_T_OVERFLOW(a,b) ((USIZE_T_MAX - (usize_t) (a)) < (usize_t) (b))
#define XOFF_T_OVERFLOW(a,b) ((XOFF_T_MAX - (xoff_t) (a)) < (xoff_t) (b))

#define MAX_LRU_SIZE 32U
#define XD3_MINSRCWINSZ (XD3_ALLOCSIZE * MAX_LRU_SIZE)
#define XD3_MAXSRCWINSZ (1ULL << 31)

#endif // XDELTA3_INTERNAL_H__
