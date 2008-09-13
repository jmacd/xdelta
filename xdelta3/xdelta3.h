/* xdelta 3 - delta compression tools and library
 * Copyright (C) 2001, 2003, 2004, 2005, 2006, 2007.  Joshua P. MacDonald
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

/* To know more about Xdelta, start by reading xdelta3.c.  If you are
 * ready to use the API, continue reading here.  There are two
 * interfaces -- xd3_encode_input and xd3_decode_input -- plus a dozen
 * or so related calls.  This interface is styled after Zlib. */

#ifndef _XDELTA3_H_
#define _XDELTA3_H_

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/****************************************************************/

/* Default configured value of stream->winsize.  If the program
 * supplies xd3_encode_input() with data smaller than winsize the
 * stream will automatically buffer the input, otherwise the input
 * buffer is used directly.
 */
#ifndef XD3_DEFAULT_WINSIZE
#define XD3_DEFAULT_WINSIZE (1U << 23)
#endif

/* Default total size of the source window used in xdelta3-main.h */
#ifndef XD3_DEFAULT_SRCWINSZ
#define XD3_DEFAULT_SRCWINSZ (1U << 26)
#endif

/* When Xdelta requests a memory allocation for certain buffers, it
 * rounds up to units of at least this size.  The code assumes (and
 * asserts) that this is a power-of-two. */
#ifndef XD3_ALLOCSIZE
#define XD3_ALLOCSIZE (1U<<14)
#endif

/* The XD3_HARDMAXWINSIZE parameter is a safety mechanism to protect
 * decoders against malicious files.  The decoder will never decode a
 * window larger than this.  If the file specifies VCD_TARGET the
 * decoder may require two buffers of this size.
 *
 * 8-16MB is reasonable, probably don't need to go larger. */
#ifndef XD3_HARDMAXWINSIZE
#define XD3_HARDMAXWINSIZE (1U<<24)
#endif
/* The IOPT_SIZE value sets the size of a buffer used to batch
 * overlapping copy instructions before they are optimized by picking
 * the best non-overlapping ranges.  The larger this buffer, the
 * longer a forced xd3_srcwin_setup() decision is held off.  Setting
 * this value to 0 causes an unlimited buffer to be used. */
#ifndef XD3_DEFAULT_IOPT_SIZE
#define XD3_DEFAULT_IOPT_SIZE    (1U<<15)
#endif

/* The maximum distance backward to search for small matches */
#ifndef XD3_DEFAULT_SPREVSZ
#define XD3_DEFAULT_SPREVSZ (1U<<18)
#endif

/* The default compression level
 */
#ifndef XD3_DEFAULT_LEVEL
#define XD3_DEFAULT_LEVEL 3
#endif

#ifndef XD3_DEFAULT_SECONDARY_LEVEL
#define XD3_DEFAULT_SECONDARY_LEVEL 6
#endif

#ifndef XD3_USE_LARGEFILE64
#define XD3_USE_LARGEFILE64 1
#endif

/* Sizes and addresses within VCDIFF windows are represented as usize_t
 *
 * For source-file offsets and total file sizes, total input and
 * output counts, the xoff_t type is used.  The decoder and encoder
 * generally check for overflow of the xoff_t size (this is tested at
 * the 32bit boundary [xdelta3-test.h]).
 */
#ifndef _WIN32
#include <stdint.h>
typedef unsigned int usize_t;
#else
#define WIN32_LEAN_AND_MEAN
#if XD3_USE_LARGEFILE64
/* 64 bit file offsets: uses GetFileSizeEx and SetFilePointerEx.
 * requires Win2000 or newer version of WinNT */
#define WINVER		0x0500
#define _WIN32_WINNT	0x0500
#else
/* 32 bit (DWORD) file offsets: uses GetFileSize and
 * SetFilePointer. compatible with win9x-me and WinNT4 */
#define WINVER		0x0400
#define _WIN32_WINNT	0x0400
#endif
#include <windows.h>
typedef unsigned int   usize_t;
#ifdef _MSC_VER
#define inline
typedef signed int     ssize_t;
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;
typedef ULONGLONG      uint64_t;
#else
/* mingw32, lcc and watcom provide a proper header */
#include <stdint.h>
#endif
#endif

/* TODO: note that SIZEOF_USIZE_T is never set to 8, although it should be for
 * a 64bit platform.  OTOH, may be that using 32bits is appropriate even on a
 * 64bit platform because we allocate large arrays of these values. */
#if XD3_USE_LARGEFILE64
#define __USE_FILE_OFFSET64 1 /* GLIBC: for 64bit fileops, ... ? */
typedef uint64_t xoff_t;
#define SIZEOF_XOFF_T 8
#define SIZEOF_USIZE_T 4
#ifndef WIN32
#define Q "ll"
#else
#define Q "I64"
#endif
#else
typedef uint32_t xoff_t;
#define SIZEOF_XOFF_T 4
#define SIZEOF_USIZE_T 4
#define Q
#endif

#define USE_UINT32 (SIZEOF_USIZE_T == 4 || \
		    SIZEOF_XOFF_T == 4 || REGRESSION_TEST)
#define USE_UINT64 (SIZEOF_USIZE_T == 8 || \
		    SIZEOF_XOFF_T == 8 || REGRESSION_TEST)

/* TODO: probably should do something better here. */
#ifndef UNALIGNED_OK
#if defined(__i386__) || defined(__i486__) || defined(__i586__) || \
  defined(__i686__) || defined(_X86_) || defined(__x86_64__)
#define UNALIGNED_OK 1
#else
#define UNALIGNED_OK 0
#endif
#endif

/**********************************************************************/

/* Whether to build the encoder, otherwise only build the decoder. */
#ifndef XD3_ENCODER
#define XD3_ENCODER 1
#endif

/* The code returned when main() fails, also defined in system
   includes. */
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

/* REGRESSION TEST enables the "xdelta3 test" command, which runs a
   series of self-tests. */
#ifndef REGRESSION_TEST
#define REGRESSION_TEST 0
#endif

/* XD3_DEBUG=1 enables assertions and various statistics.  Levels > 1
 * enable some additional output only useful during development and
 * debugging. */
#ifndef XD3_DEBUG
#define XD3_DEBUG 0
#endif

#ifndef PYTHON_MODULE
#define PYTHON_MODULE 0
#endif

#ifndef SWIG_MODULE
#define SWIG_MODULE 0
#endif

/* There are three string matching functions supplied: one fast, one
 * slow (default), and one soft-configurable.  To disable any of
 * these, use the following definitions. */
#ifndef XD3_BUILD_SLOW
#define XD3_BUILD_SLOW 1
#endif
#ifndef XD3_BUILD_FAST
#define XD3_BUILD_FAST 1
#endif
#ifndef XD3_BUILD_FASTER
#define XD3_BUILD_FASTER 1
#endif
#ifndef XD3_BUILD_FASTEST
#define XD3_BUILD_FASTEST 1
#endif
#ifndef XD3_BUILD_SOFT
#define XD3_BUILD_SOFT 1
#endif
#ifndef XD3_BUILD_DEFAULT
#define XD3_BUILD_DEFAULT 1
#endif

#if XD3_DEBUG
#include <stdio.h>
#endif

/* XPRINT.  Debug output and VCDIFF_TOOLS functions report to stderr.
 * I have used an irregular style to abbreviate [fprintf(stderr, "] as
 * [DP(RINT "]. */
#define DP   fprintf
#define RINT stderr,

typedef struct _xd3_stream             xd3_stream;
typedef struct _xd3_source             xd3_source;
typedef struct _xd3_hash_cfg           xd3_hash_cfg;
typedef struct _xd3_smatcher           xd3_smatcher;
typedef struct _xd3_rinst              xd3_rinst;
typedef struct _xd3_dinst              xd3_dinst;
typedef struct _xd3_hinst              xd3_hinst;
typedef struct _xd3_winst              xd3_winst;
typedef struct _xd3_rpage              xd3_rpage;
typedef struct _xd3_addr_cache         xd3_addr_cache;
typedef struct _xd3_output             xd3_output;
typedef struct _xd3_desect             xd3_desect;
typedef struct _xd3_iopt_buflist       xd3_iopt_buflist;
typedef struct _xd3_rlist              xd3_rlist;
typedef struct _xd3_sec_type           xd3_sec_type;
typedef struct _xd3_sec_cfg            xd3_sec_cfg;
typedef struct _xd3_sec_stream         xd3_sec_stream;
typedef struct _xd3_config             xd3_config;
typedef struct _xd3_code_table_desc    xd3_code_table_desc;
typedef struct _xd3_code_table_sizes   xd3_code_table_sizes;
typedef struct _xd3_slist              xd3_slist;
typedef struct _xd3_whole_state        xd3_whole_state;
typedef struct _xd3_wininfo            xd3_wininfo;

/* The stream configuration has three callbacks functions, all of
 * which may be supplied with NULL values.  If config->getblk is
 * provided as NULL, the stream returns XD3_GETSRCBLK. */

typedef void*  (xd3_alloc_func)    (void       *opaque,
				    usize_t      items,
				    usize_t      size);
typedef void   (xd3_free_func)     (void       *opaque,
				    void       *address);

typedef int    (xd3_getblk_func)   (xd3_stream *stream,
				    xd3_source *source,
				    xoff_t      blkno);

/* These are internal functions to delay construction of encoding
 * tables and support alternate code tables.  See the comments & code
 * enabled by GENERIC_ENCODE_TABLES. */

typedef const xd3_dinst* (xd3_code_table_func) (void);
typedef int              (xd3_comp_table_func) (xd3_stream *stream,
						const uint8_t **data,
						usize_t *size);



#if XD3_DEBUG
#define XD3_ASSERT(x) \
    do { if (! (x)) { DP(RINT "%s:%d: XD3 assertion failed: %s\n", __FILE__, __LINE__, #x); \
    abort (); } } while (0)
#else
#define XD3_ASSERT(x) (void)0
#endif

#ifdef __GNUC__
/* As seen on linux-kernel. */
#ifndef max
#define max(x,y) ({ \
	const typeof(x) _x = (x);	\
	const typeof(y) _y = (y);	\
	(void) (&_x == &_y);		\
	_x > _y ? _x : _y; })
#endif

#ifndef min
#define min(x,y) ({ \
	const typeof(x) _x = (x);	\
	const typeof(y) _y = (y);	\
	(void) (&_x == &_y);		\
	_x < _y ? _x : _y; })
#endif
#else
#ifndef max
#define max(x,y) ((x) < (y) ? (y) : (x))
#endif
#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif
#endif

/****************************************************************
 PUBLIC ENUMS
 ******************************************************************/

/* These are the five ordinary status codes returned by the
 * xd3_encode_input() and xd3_decode_input() state machines. */
typedef enum {

  /* An application must be prepared to handle these five return
   * values from either xd3_encode_input or xd3_decode_input, except
   * in the case of no-source compression, in which case XD3_GETSRCBLK
   * is never returned.  More detailed comments for these are given in
   * xd3_encode_input and xd3_decode_input comments, below. */
  XD3_INPUT     = -17703, /* need input */
  XD3_OUTPUT    = -17704, /* have output */
  XD3_GETSRCBLK = -17705, /* need a block of source input (with no
			   * xd3_getblk function), a chance to do
			   * non-blocking read. */
  XD3_GOTHEADER = -17706, /* (decode-only) after the initial VCDIFF &
			     first window header */
  XD3_WINSTART  = -17707, /* notification: returned before a window is
			   * processed, giving a chance to
			   * XD3_SKIP_WINDOW or not XD3_SKIP_EMIT that
			   * window. */
  XD3_WINFINISH  = -17708, /* notification: returned after
			      encode/decode & output for a window */
  XD3_TOOFARBACK = -17709, /* (encoder only) may be returned by
			      getblk() if the block is too old */
  XD3_INTERNAL   = -17710, /* internal error */
  XD3_INVALID    = -17711, /* invalid config */
  XD3_INVALID_INPUT = -17712, /* invalid input/decoder error */
  XD3_NOSECOND  = -17713, /* when secondary compression finds no
			     improvement. */

} xd3_rvalues;

/* special values in config->flags */
typedef enum
{
  XD3_JUST_HDR       = (1 << 1),   /* used by VCDIFF tools, see
				      xdelta3-main.h. */
  XD3_SKIP_WINDOW    = (1 << 2),   /* used by VCDIFF tools, see
				      xdelta3-main.h. */
  XD3_SKIP_EMIT      = (1 << 3),   /* used by VCDIFF tools, see
				      xdelta3-main.h. */
  XD3_FLUSH          = (1 << 4),   /* flush the stream buffer to
				      prepare for
				      xd3_stream_close(). */

  XD3_SEC_DJW        = (1 << 5),   /* use DJW static huffman */
  XD3_SEC_FGK        = (1 << 6),   /* use FGK adaptive huffman */
  XD3_SEC_TYPE       = (XD3_SEC_DJW | XD3_SEC_FGK),

  XD3_SEC_NODATA     = (1 << 7),   /* disable secondary compression of
				      the data section. */
  XD3_SEC_NOINST     = (1 << 8),   /* disable secondary compression of
				      the inst section. */
  XD3_SEC_NOADDR     = (1 << 9),   /* disable secondary compression of
				      the addr section. */

  XD3_SEC_NOALL      = (XD3_SEC_NODATA | XD3_SEC_NOINST | XD3_SEC_NOADDR),

  XD3_ADLER32        = (1 << 10),  /* enable checksum computation in
				      the encoder. */
  XD3_ADLER32_NOVER  = (1 << 11),  /* disable checksum verification in
				      the decoder. */

  XD3_ALT_CODE_TABLE = (1 << 12),  /* for testing th
				      e alternate code table encoding. */

  XD3_NOCOMPRESS     = (1 << 13),  /* disable ordinary data
				    * compression feature, only search
				    * the source, not the target. */
  XD3_BEGREEDY       = (1 << 14),  /* disable the "1.5-pass
				    * algorithm", instead use greedy
				    * matching.  Greedy is off by
				    * default. */
  XD3_ADLER32_RECODE = (1 << 15),  /* used by "recode". */

  /* 4 bits to set the compression level the same as the command-line
   * setting -1 through -9 (-0 corresponds to the XD3_NOCOMPRESS flag,
   * and is independent of compression level).  This is for
   * convenience, especially with xd3_encode_memory(). */

  XD3_COMPLEVEL_SHIFT = 20,  /* 20 - 24 */
  XD3_COMPLEVEL_MASK = (0xF << XD3_COMPLEVEL_SHIFT),
  XD3_COMPLEVEL_1 = (1 << XD3_COMPLEVEL_SHIFT),
  XD3_COMPLEVEL_2 = (2 << XD3_COMPLEVEL_SHIFT),
  XD3_COMPLEVEL_3 = (3 << XD3_COMPLEVEL_SHIFT),
  XD3_COMPLEVEL_6 = (6 << XD3_COMPLEVEL_SHIFT),
  XD3_COMPLEVEL_9 = (9 << XD3_COMPLEVEL_SHIFT),

} xd3_flags;

/* The values of this enumeration are set in xd3_config using the
 * smatch_cfg variable.  It can be set to default, slow, fast, etc.,
 * and soft. */
typedef enum
{
  XD3_SMATCH_DEFAULT = 0, /* Flags may contain XD3_COMPLEVEL bits,
			     else default. */
  XD3_SMATCH_SLOW    = 1,
  XD3_SMATCH_FAST    = 2,
  XD3_SMATCH_FASTER  = 3,
  XD3_SMATCH_FASTEST = 4,
  XD3_SMATCH_SOFT    = 5,
} xd3_smatch_cfg;

/*********************************************************************
 PRIVATE ENUMS
**********************************************************************/

/* stream->match_state is part of the xd3_encode_input state machine
 *  for source matching:
 *
 *  1. the XD3_GETSRCBLK block-read mechanism means reentrant matching
 *  2. this state spans encoder windows: a match and end-of-window
 *  will continue in the next 3. the initial target byte and source
 *  byte are a presumed match, to avoid some computation in case the
 *  inputs are identical.
 */
typedef enum {

  MATCH_TARGET    = 0, /* in this state, attempt to match the start of
			* the target with the previously set source
			* address (initially 0). */
  MATCH_BACKWARD  = 1, /* currently expanding a match backward in the
			  source/target. */
  MATCH_FORWARD   = 2, /* currently expanding a match forward in the
			  source/target. */
  MATCH_SEARCHING = 3, /* currently searching for a match. */

} xd3_match_state;

/* The xd3_encode_input state machine steps through these states in
 * the following order.  The matcher is reentrant and returns
 * XD3_INPUT whenever it requires more data.  After receiving
 * XD3_INPUT, if the application reads EOF it should call
 * xd3_stream_close().
 */
typedef enum {

  ENC_INIT      = 0, /* xd3_encode_input has never been called. */
  ENC_INPUT     = 1, /* waiting for xd3_avail_input () to be called. */
  ENC_SEARCH    = 2, /* currently searching for matches. */
  ENC_INSTR     = 3, /* currently formatting output. */
  ENC_FLUSH     = 4, /* currently emitting output. */
  ENC_POSTOUT   = 5, /* after an output section. */
  ENC_POSTWIN   = 6, /* after all output sections. */
  ENC_ABORTED   = 7, /* abort. */
} xd3_encode_state;

/* The xd3_decode_input state machine steps through these states in
 * the following order.  The matcher is reentrant and returns
 * XD3_INPUT whenever it requires more data.  After receiving
 * XD3_INPUT, if the application reads EOF it should call
 * xd3_stream_close().
 *
 * 0-8:   the VCDIFF header
 * 9-18:  the VCDIFF window header
 * 19-21: the three primary sections: data, inst, addr
 * 22:    producing output: returns XD3_OUTPUT, possibly XD3_GETSRCBLK,
 * 23:    return XD3_WINFINISH, set state=9 to decode more input
 */
typedef enum {

  DEC_VCHEAD   = 0, /* VCDIFF header */
  DEC_HDRIND   = 1, /* header indicator */

  DEC_SECONDID = 2, /* secondary compressor ID */

  DEC_TABLEN   = 3, /* code table length */
  DEC_NEAR     = 4, /* code table near */
  DEC_SAME     = 5, /* code table same */
  DEC_TABDAT   = 6, /* code table data */

  DEC_APPLEN   = 7, /* application data length */
  DEC_APPDAT   = 8, /* application data */

  DEC_WININD   = 9, /* window indicator */

  DEC_CPYLEN   = 10, /* copy window length */
  DEC_CPYOFF   = 11, /* copy window offset */

  DEC_ENCLEN   = 12, /* length of delta encoding */
  DEC_TGTLEN   = 13, /* length of target window */
  DEC_DELIND   = 14, /* delta indicator */

  DEC_DATALEN  = 15, /* length of ADD+RUN data */
  DEC_INSTLEN  = 16, /* length of instruction data */
  DEC_ADDRLEN  = 17, /* length of address data */

  DEC_CKSUM    = 18, /* window checksum */

  DEC_DATA     = 19, /* data section */
  DEC_INST     = 20, /* instruction section */
  DEC_ADDR     = 21, /* address section */

  DEC_EMIT     = 22, /* producing data */

  DEC_FINISH   = 23, /* window finished */

  DEC_ABORTED  = 24, /* xd3_abort_stream */
} xd3_decode_state;

/************************************************************
 internal types
 ************************************************************/

/* instruction lists used in the IOPT buffer */
struct _xd3_rlist
{
  xd3_rlist  *next;
  xd3_rlist  *prev;
};

/* the raw encoding of an instruction used in the IOPT buffer */
struct _xd3_rinst
{
  uint8_t     type;
  uint8_t     xtra;
  uint8_t     code1;
  uint8_t     code2;
  usize_t      pos;
  usize_t      size;
  xoff_t      addr;
  xd3_rlist   link;
};

/* the code-table form of an single- or double-instruction */
struct _xd3_dinst
{
  uint8_t     type1;
  uint8_t     size1;
  uint8_t     type2;
  uint8_t     size2;
};

/* the decoded form of a single (half) instruction. */
struct _xd3_hinst
{
  uint8_t     type;
  uint32_t    size;  /* TODO: why decode breaks if this is usize_t? */
  uint32_t    addr;  /* TODO: why decode breaks if this is usize_t? */
};

/* the form of a whole-file instruction */
struct _xd3_winst
{
  uint8_t type;  /* RUN, ADD, COPY */
  uint8_t mode;  /* 0, VCD_SOURCE, VCD_TARGET */
  usize_t size;
  xoff_t  addr;
  xoff_t  position;  /* absolute position of this inst */
};

/* used by the encoder to buffer output in sections.  list of blocks. */
struct _xd3_output
{
  uint8_t    *base;
  usize_t     next;
  usize_t     avail;
  xd3_output *next_page;
};

/* used by the decoder to buffer input in sections. */
struct _xd3_desect
{
  const uint8_t *buf;
  const uint8_t *buf_max;
  uint32_t       size;  /* TODO: why decode breaks if this is usize_t? */
  usize_t        pos;

  /* used in xdelta3-decode.h */
  uint8_t       *copied1;
  usize_t        alloc1;

  /* used in xdelta3-second.h */
  uint8_t       *copied2;
  usize_t        alloc2;
};

/* the VCDIFF address cache, see the RFC */
struct _xd3_addr_cache
{
  usize_t  s_near;
  usize_t  s_same;
  usize_t  next_slot;  /* the circular index for near */
  usize_t *near_array; /* array of size s_near        */
  usize_t *same_array; /* array of size s_same*256    */
};

/* the IOPT buffer list is just a list of buffers, which may be allocated
 * during encode when using an unlimited buffer. */
struct _xd3_iopt_buflist
{
  xd3_rinst *buffer;
  xd3_iopt_buflist *next;
};

/* This is the record of a pre-compiled configuration, a subset of
   xd3_config. */
struct _xd3_smatcher
{
  const char        *name;
  int             (*string_match) (xd3_stream  *stream);
  usize_t            large_look;
  usize_t            large_step;
  usize_t            small_look;
  usize_t            small_chain;
  usize_t            small_lchain;
  usize_t            max_lazy;
  usize_t            long_enough;
};

/* hash table size & power-of-two hash function. */
struct _xd3_hash_cfg
{
  usize_t           size;
  usize_t           shift;
  usize_t           mask;
};

/* the sprev list */
struct _xd3_slist
{
  usize_t     last_pos;
};

/* window info (for whole state) */
struct _xd3_wininfo {
  xoff_t offset;
  usize_t length;
  uint32_t adler32;
};

/* whole state for, e.g., merge */
struct _xd3_whole_state {
  usize_t addslen;
  uint8_t *adds;
  usize_t  adds_alloc;

  usize_t instlen;
  xd3_winst *inst;
  usize_t  inst_alloc;

  usize_t wininfolen;
  xd3_wininfo *wininfo;
  usize_t wininfo_alloc;

  xoff_t length;
};

/********************************************************************
 public types
 *******************************************************************/

/* Settings for the secondary compressor. */
struct _xd3_sec_cfg
{
  int                data_type;     /* Which section. (set automatically) */
  int                ngroups;       /* Number of DJW Huffman groups. */
  int                sector_size;   /* Sector size. */
  int                inefficient;   /* If true, ignore efficiency check [avoid XD3_NOSECOND]. */
};

/* This is the user-visible stream configuration. */
struct _xd3_config
{
  usize_t             winsize;       /* The encoder window size. */
  usize_t             sprevsz;       /* How far back small string
					matching goes */
  usize_t             iopt_size;     /* entries in the
					instruction-optimizing
					buffer */
  usize_t             srcwin_maxsz;  /* srcwin_size grows by a factor
					of 2 when no matches are
					found */

  xd3_getblk_func   *getblk;        /* The three callbacks. */
  xd3_alloc_func    *alloc;
  xd3_free_func     *freef;
  void              *opaque;        /* Not used. */
  int                flags;         /* stream->flags are initialized
				     * from xd3_config & never
				     * modified by the library.  Use
				     * xd3_set_flags to modify flags
				     * settings mid-stream. */

  xd3_sec_cfg       sec_data;       /* Secondary compressor config: data */
  xd3_sec_cfg       sec_inst;       /* Secondary compressor config: inst */
  xd3_sec_cfg       sec_addr;       /* Secondary compressor config: addr */

  xd3_smatch_cfg     smatch_cfg;    /* See enum: use fields below  for
				       soft config */
  xd3_smatcher       smatcher_soft;
};

/* The primary source file object. You create one of these objects and
 * initialize the first four fields.  This library maintains the next
 * 5 fields.  The configured getblk implementation is responsible for
 * setting the final 3 fields when called (and/or when XD3_GETSRCBLK
 * is returned).
 */
struct _xd3_source
{
  /* you set */
  xoff_t              size;          /* size of this source */
  usize_t             blksize;       /* block size */
  const char         *name;          /* its name, for debug/print
					purposes */
  void               *ioh;           /* opaque handle */

  /* getblk sets */
  xoff_t              curblkno;      /* current block number: client
					sets after getblk request */
  usize_t             onblk;         /* number of bytes on current
					block: client sets, xd3
					verifies */
  const uint8_t      *curblk;        /* current block array: client
					sets after getblk request */

  /* xd3 sets */
  usize_t             srclen;        /* length of this source window */
  xoff_t              srcbase;       /* offset of this source window
					in the source itself */
  xoff_t              blocks;        /* the total number of blocks in
					this source */
  usize_t             onlastblk;     /* cached size info, avoid __udivdi3 */
  int                 shiftby;       /* for power-of-two blocksizes */
  int                 maskby;        /* for power-of-two blocksizes */  
  xoff_t              cpyoff_blocks; /* offset of dec_cpyoff in blocks */
  usize_t             cpyoff_blkoff; /* offset of copy window in
					blocks, remainder */
  xoff_t              getblkno;      /* request block number: xd3 sets
					current getblk request */
};

/* The primary xd3_stream object, used for encoding and decoding.  You
 * may access only two fields: avail_out, next_out.  Use the methods
 * above to operate on xd3_stream. */
struct _xd3_stream
{
  /* input state */
  const uint8_t    *next_in;          /* next input byte */
  usize_t           avail_in;         /* number of bytes available at
					 next_in */
  xoff_t            total_in;         /* how many bytes in */

  /* output state */
  uint8_t          *next_out;         /* next output byte */
  usize_t           avail_out;        /* number of bytes available at
					 next_out */
  usize_t           space_out;        /* total out space */
  xoff_t            current_window;   /* number of windows encoded/decoded */
  xoff_t            total_out;        /* how many bytes out */

  /* to indicate an error, xd3 sets */
  const char       *msg;              /* last error message, NULL if
					 no error */

  /* source configuration */
  xd3_source       *src;              /* source array */

  /* encoder memory configuration */
  usize_t           winsize;          /* suggested window size */
  usize_t           sprevsz;          /* small string, previous window
					 size (power of 2) */
  usize_t           sprevmask;        /* small string, previous window
					 size mask */
  usize_t           iopt_size;
  usize_t           iopt_unlimited;
  usize_t           srcwin_maxsz;

  /* general configuration */
  xd3_getblk_func  *getblk;           /* set nxtblk, nxtblkno to scanblkno */
  xd3_alloc_func   *alloc;            /* malloc function */
  xd3_free_func    *free;             /* free function */
  void*             opaque;           /* private data object passed to
					 alloc, free, and getblk */
  int               flags;            /* various options */
  
  /* secondary compressor configuration */
  xd3_sec_cfg       sec_data;         /* Secondary compressor config: data */
  xd3_sec_cfg       sec_inst;         /* Secondary compressor config: inst */
  xd3_sec_cfg       sec_addr;         /* Secondary compressor config: addr */

  xd3_smatcher      smatcher;

  usize_t           *large_table;      /* table of large checksums */
  xd3_hash_cfg       large_hash;       /* large hash config */

  usize_t           *small_table;      /* table of small checksums */
  xd3_slist         *small_prev;       /* table of previous offsets,
					  circular linked list */
  int                small_reset;      /* true if small table should
					  be reset */

  xd3_hash_cfg       small_hash;       /* small hash config */
  xd3_addr_cache     acache;           /* the vcdiff address cache */
  xd3_encode_state   enc_state;        /* state of the encoder */

  usize_t            taroff;           /* base offset of the target input */
  usize_t            input_position;   /* current input position */
  usize_t            min_match;        /* current minimum match
					  length, avoids redundent
					  matches */
  usize_t            unencoded_offset; /* current input, first
				       * unencoded offset. this value
				       * is <= the first instruction's
				       * position in the iopt buffer,
				       * if there is at least one
				       * match in the buffer. */

  // SRCWIN
  // these variables plus srcwin_maxsz above (set by config)
  int                srcwin_decided;    /* boolean: true if the
					   srclen,srcbase have been
					   decided. */
  xoff_t             srcwin_cksum_pos;  /* Source checksum position */

  // MATCH
  xd3_match_state    match_state;      /* encoder match state */
  xoff_t             match_srcpos;     /* current match source
					  position relative to
					  srcbase */
  xoff_t             match_last_srcpos;  /* previously attempted
					  * srcpos, to avoid loops. */
  xoff_t             match_minaddr;    /* smallest matching address to
				       * set window params (reset each
				       * window xd3_encode_reset) */
  xoff_t             match_maxaddr;    /* largest matching address to
				       * set window params (reset each
				       * window xd3_encode_reset) */
  usize_t            match_back;       /* match extends back so far */
  usize_t            match_maxback;    /* match extends back maximum */
  usize_t            match_fwd;        /* match extends forward so far */
  usize_t            match_maxfwd;     /* match extends forward maximum */

  xoff_t             maxsrcaddr;      /* address of the last source
					 match (across windows) */

  uint8_t          *buf_in;           /* for saving buffered input */
  usize_t            buf_avail;        /* amount of saved input */
  const uint8_t    *buf_leftover;     /* leftover content of next_in
					 (i.e., user's buffer) */
  usize_t            buf_leftavail;    /* amount of leftover content */

  xd3_output       *enc_current;      /* current output buffer */
  xd3_output       *enc_free;         /* free output buffers */
  xd3_output       *enc_heads[4];     /* array of encoded outputs:
					 head of chain */
  xd3_output       *enc_tails[4];     /* array of encoded outputs:
					 tail of chain */
  uint32_t          recode_adler32;   /* set the adler32 checksum
				       * during "recode". */

  xd3_rlist         iopt_used;        /* instruction optimizing buffer */
  xd3_rlist         iopt_free;
  xd3_rinst        *iout;             /* next single instruction */
  xd3_iopt_buflist *iopt_alloc;

  const uint8_t    *enc_appheader;    /* application header to encode */
  usize_t            enc_appheadsz;    /* application header size */

  /* decoder stuff */
  xd3_decode_state  dec_state;        /* current DEC_XXX value */
  usize_t           dec_hdr_ind;      /* VCDIFF header indicator */
  usize_t           dec_win_ind;      /* VCDIFF window indicator */
  usize_t           dec_del_ind;      /* VCDIFF delta indicator */

  uint8_t           dec_magic[4];     /* First four bytes */
  usize_t           dec_magicbytes;   /* Magic position. */

  usize_t           dec_secondid;     /* Optional secondary compressor ID. */

  /* TODO: why decode breaks if this is usize_t? */
  uint32_t          dec_codetblsz;    /* Optional code table: length. */
  uint8_t          *dec_codetbl;      /* Optional code table: storage. */
  usize_t           dec_codetblbytes; /* Optional code table: position. */

  /* TODO: why decode breaks if this is usize_t? */
  uint32_t          dec_appheadsz;    /* Optional application header:
					 size. */
  uint8_t          *dec_appheader;    /* Optional application header:
					 storage */
  usize_t           dec_appheadbytes; /* Optional application header:
					 position. */

  usize_t            dec_cksumbytes;   /* Optional checksum: position. */
  uint8_t           dec_cksum[4];     /* Optional checksum: storage. */
  uint32_t          dec_adler32;      /* Optional checksum: value. */

  /* TODO: why decode breaks if this is usize_t? */
  uint32_t           dec_cpylen;       /* length of copy window
					  (VCD_SOURCE or VCD_TARGET) */
  xoff_t             dec_cpyoff;       /* offset of copy window
					  (VCD_SOURCE or VCD_TARGET) */
  /* TODO: why decode breaks if this is usize_t? */
  uint32_t           dec_enclen;       /* length of delta encoding */
  /* TODO: why decode breaks if this is usize_t? */
  uint32_t           dec_tgtlen;       /* length of target window */

#if USE_UINT64
  uint64_t          dec_64part;       /* part of a decoded uint64_t */
#endif
#if USE_UINT32
  uint32_t          dec_32part;       /* part of a decoded uint32_t */
#endif

  xoff_t            dec_winstart;     /* offset of the start of
                                         current target window */
  xoff_t            dec_window_count; /* == current_window + 1 in
                                         DEC_FINISH */
  usize_t            dec_winbytes;     /* bytes of the three sections
                                          so far consumed */
  usize_t            dec_hdrsize;      /* VCDIFF + app header size */

  const uint8_t    *dec_tgtaddrbase;  /* Base of decoded target
                                         addresses (addr >=
                                         dec_cpylen). */
  const uint8_t    *dec_cpyaddrbase;  /* Base of decoded copy
                                         addresses (addr <
                                         dec_cpylen). */

  usize_t            dec_position;     /* current decoder position
                                          counting the cpylen
                                          offset */
  usize_t            dec_maxpos;       /* maximum decoder position
                                          counting the cpylen
                                          offset */
  xd3_hinst         dec_current1;     /* current instruction */
  xd3_hinst         dec_current2;     /* current instruction */

  uint8_t          *dec_buffer;       /* Decode buffer */
  uint8_t          *dec_lastwin;      /* In case of VCD_TARGET, the
                                         last target window. */
  usize_t            dec_lastlen;      /* length of the last target
                                          window */
  xoff_t            dec_laststart;    /* offset of the start of last
                                         target window */
  usize_t            dec_lastspace;    /* allocated space of last
                                          target window, for reuse */

  xd3_desect        inst_sect;        /* staging area for decoding
                                         window sections */
  xd3_desect        addr_sect;
  xd3_desect        data_sect;

  xd3_code_table_func       *code_table_func;
  xd3_comp_table_func       *comp_table_func;
  const xd3_dinst           *code_table;
  const xd3_code_table_desc *code_table_desc;
  xd3_dinst                 *code_table_alloc;

  /* secondary compression */
  const xd3_sec_type *sec_type;
  xd3_sec_stream     *sec_stream_d;
  xd3_sec_stream     *sec_stream_i;
  xd3_sec_stream     *sec_stream_a;

  /* state for reconstructing whole files (e.g., for merge), this only
   * supports loading USIZE_T_MAX instructions, adds, etc. */
  xd3_whole_state     whole_target;

  /* statistics */
  xoff_t            n_scpy;
  xoff_t            n_tcpy;
  xoff_t            n_add;
  xoff_t            n_run;

  xoff_t            l_scpy;
  xoff_t            l_tcpy;
  xoff_t            l_add;
  xoff_t            l_run;

  usize_t           i_slots_used;

#if XD3_DEBUG
  usize_t            large_ckcnt;

  /* memory usage */
  usize_t            alloc_cnt;
  usize_t            free_cnt;
#endif
};

/**************************************************************************
 PUBLIC FUNCTIONS
 **************************************************************************/

/* This function configures an xd3_stream using the provided in-memory
 * input buffer, source buffer, output buffer, and flags.  The output
 * array must be large enough or else ENOSPC will be returned.  This
 * is the simplest in-memory encoding interface. */
int     xd3_encode_memory (const uint8_t *input,
			   usize_t        input_size,
			   const uint8_t *source,
			   usize_t        source_size,
			   uint8_t       *output_buffer,
			   usize_t       *output_size,
			   usize_t        avail_output,
			   int            flags);

/* The reverse of xd3_encode_memory. */
int     xd3_decode_memory (const uint8_t *input,
			   usize_t        input_size,
			   const uint8_t *source,
			   usize_t        source_size,
			   uint8_t       *output_buf,
			   usize_t       *output_size,
			   usize_t        avail_output,
			   int            flags);

/* This function encodes an in-memory input.  Everything else about
 * the xd3_stream is configurable.  The output array must be large
 * enough to hold the output or else ENOSPC is returned.  The source
 * (if any) should be set using xd3_set_source() with a single-block
 * xd3_source.  This calls the underlying non-blocking interface,
 * xd3_encode_input(), handling the necessary input/output states.
 * This method be considered a reference for any application using
 * xd3_encode_input() directly.
 *
 *   xd3_stream stream;
 *   xd3_config config;
 *   xd3_source src;
 * 
 *   memset (& src, 0, sizeof (src));
 *   memset (& stream, 0, sizeof (stream));
 *   memset (& config, 0, sizeof (config));
 *
 *   if (source != NULL)
 *     {
 *       src.size = source_size;
 *       src.blksize = source_size;
 *       src.curblkno = 0;
 *       src.onblk = source_size;
 *       src.curblk = source;
 *       xd3_set_source(&stream, &src);
 *     }
 *
 *   config.flags = flags;
 *   config.srcwin_maxsz = source_size;
 *   config.winsize = input_size;
 *
 *   ... set smatcher, appheader, encoding-table, compression-level, etc.
 *
 *   xd3_config_stream(&stream, &config);
 *   xd3_encode_stream(&stream, ...);
 *   xd3_free_stream(&stream);
 *
 * DO NOT USE except for testing. These methods are allocate bad buffer sizes.
 */
int     xd3_encode_stream (xd3_stream    *stream,
			   const uint8_t *input,
			   usize_t         input_size,
			   uint8_t       *output,
			   usize_t        *output_size,
			   usize_t         avail_output);

/* The reverse of xd3_encode_stream. */
int     xd3_decode_stream (xd3_stream    *stream,
			   const uint8_t *input,
			   usize_t        input_size,
			   uint8_t       *output,
			   usize_t       *output_size,
			   usize_t        avail_size);

/* This is the non-blocking interface.
 *
 * Handling input and output states is the same for encoding or
 * decoding using the xd3_avail_input() and xd3_consume_output()
 * routines, inlined below.
 *
 * Return values:
 *
 *   XD3_INPUT: the process requires more input: call
 *               xd3_avail_input() then repeat
 *
 *   XD3_OUTPUT: the process has more output: read stream->next_out,
 *               stream->avail_out, then call xd3_consume_output(),
 *               then repeat
 *
 *   XD3_GOTHEADER: (decoder-only) notification returned following the
 *               VCDIFF header and first window header.  the decoder
 *               may use the header to configure itself.
 *
 *   XD3_WINSTART: a general notification returned once for each
 *               window except the 0-th window, which is implied by
 *               XD3_GOTHEADER.  It is recommended to use a
 *               switch-stmt such as:
 *
 *                 ...
 *               again:
 *                 switch ((ret = xd3_decode_input (stream))) {
 *                    case XD3_GOTHEADER: {
 *                      assert(stream->current_window == 0);
 *                      stuff;
 *                    }
 *                    // fallthrough 
 *                    case XD3_WINSTART: {
 *                      something(stream->current_window);
 *                      goto again;
 *                    }
 *                    ...
 *
 *   XD3_WINFINISH: a general notification, following the complete
 *               input & output of a window.  at this point,
 *               stream->total_in and stream->total_out are consistent
 *               for either encoding or decoding.
 *
 *   XD3_GETSRCBLK: If the xd3_getblk() callback is NULL, this value
 *               is returned to initiate a non-blocking source read.
 */
int     xd3_decode_input  (xd3_stream    *stream);
int     xd3_encode_input  (xd3_stream    *stream);

/* The xd3_config structure is used to initialize a stream - all data
 * is copied into stream so config may be a temporary variable.  See
 * the [documentation] or comments on the xd3_config structure. */
int     xd3_config_stream (xd3_stream    *stream,
			   xd3_config    *config);

/* Since Xdelta3 doesn't open any files, xd3_close_stream is just an
 * error check that the stream is in a proper state to be closed: this
 * means the encoder is flushed and the decoder is at a window
 * boundary.  The application is responsible for freeing any of the
 * resources it supplied. */
int     xd3_close_stream (xd3_stream    *stream);

/* This arranges for closes the stream to succeed.  Does not free the
 * stream.*/
void    xd3_abort_stream (xd3_stream    *stream);

/* xd3_free_stream frees all memory allocated for the stream.  The
 * application is responsible for freeing any of the resources it
 * supplied. */
void    xd3_free_stream   (xd3_stream    *stream);

/* This function informs the encoder or decoder that source matching
 * (i.e., delta-compression) is possible.  For encoding, this should
 * be called before the first xd3_encode_input.  A NULL source is
 * ignored.  For decoding, this should be called before the first
 * window is decoded, but the appheader may be read first
 * (XD3_GOTHEADER).  After decoding the header, call xd3_set_source()
 * if you have a source file.  Note: if (stream->dec_win_ind & VCD_SOURCE)
 * is true, it means the first window expects there to be a source file.
 */
int     xd3_set_source    (xd3_stream    *stream,
			   xd3_source    *source);

/* This should be called before the first call to xd3_encode_input()
 * to include application-specific data in the VCDIFF header. */
void    xd3_set_appheader (xd3_stream    *stream,
			   const uint8_t *data,
			   usize_t        size);

/* xd3_get_appheader may be called in the decoder after XD3_GOTHEADER.
 * For convenience, the decoder always adds a single byte padding to
 * the end of the application header, which is set to zero in case the
 * application header is a string. */
int     xd3_get_appheader (xd3_stream     *stream,
			   uint8_t       **data,
			   usize_t        *size);

/* To generate a VCDIFF encoded delta with xd3_encode_init() from
 * another format, use:
 *
 *   xd3_encode_init_partial() -- initialze encoder state (w/o hash tables)
 *   xd3_init_cache() -- reset VCDIFF address cache
 *   xd3_found_match() -- to report a copy instruction
 *
 * set stream->enc_state to ENC_INSTR and call xd3_encode_input as usual.
 */
int xd3_encode_init_partial (xd3_stream *stream);
void xd3_init_cache (xd3_addr_cache* acache);
int xd3_found_match (xd3_stream *stream,
		     usize_t pos, usize_t size,
		     xoff_t addr, int is_source);

/* Gives an error string for xdelta3-speficic errors, returns NULL for
   system errors */
const char* xd3_strerror (int ret);

/* For convenience, zero & initialize the xd3_config structure with
   specified flags. */
static inline
void    xd3_init_config (xd3_config *config,
			 int         flags)
{
  memset (config, 0, sizeof (*config));
  config->flags = flags;
}

/* This supplies some input to the stream. */
static inline
void    xd3_avail_input  (xd3_stream    *stream,
			  const uint8_t *idata,
			  usize_t         isize)
{
  /* Even if isize is zero, the code expects a non-NULL idata.  Why?
   * It uses this value to determine whether xd3_avail_input has ever
   * been called.  If xd3_encode_input is called before
   * xd3_avail_input it will return XD3_INPUT right away without
   * allocating a stream->winsize buffer.  This is to avoid an
   * unwanted allocation. */
  XD3_ASSERT (idata != NULL || isize == 0);

  stream->next_in  = idata;
  stream->avail_in = isize;
}

/* This acknowledges receipt of output data, must be called after any
 * XD3_OUTPUT return. */
static inline
void xd3_consume_output (xd3_stream  *stream)
{
  stream->avail_out  = 0;
}

/* These are set for each XD3_WINFINISH return. */
static inline
int xd3_encoder_used_source (xd3_stream *stream) {
  return stream->src != NULL && stream->src->srclen > 0;
}
static inline
xoff_t xd3_encoder_srcbase (xd3_stream *stream) {
  return stream->src->srcbase;
}
static inline
usize_t xd3_encoder_srclen (xd3_stream *stream) {
  return stream->src->srclen;
}

/* Checks for legal flag changes. */
static inline
void xd3_set_flags (xd3_stream *stream, int flags)
{
  /* The bitwise difference should contain only XD3_FLUSH or
     XD3_SKIP_WINDOW */
  XD3_ASSERT(((flags ^ stream->flags) & ~(XD3_FLUSH | XD3_SKIP_WINDOW)) == 0);
  stream->flags = flags;
}

/* Gives some extra information about the latest library error, if any
   is known. */
static inline
const char* xd3_errstring (xd3_stream  *stream)
{
  return stream->msg ? stream->msg : "";
}


/* 64-bit divisions are expensive.  on a 32bit platform, these show in
 * a profile as __udivdi3().  these are all the xoff_t divisions: */
static inline
void xd3_blksize_div (const xoff_t offset,
		      const xd3_source *source,
		      xoff_t *blkno,
		      usize_t *blkoff) {
  *blkno = source->maskby ?
    (offset >> source->shiftby) :
    (offset / source->blksize);
  *blkoff = source->maskby ?
    (offset & source->maskby) :
    (offset - *blkno * source->blksize);
}

/* This function tells the number of bytes expected to be set in
 * source->onblk after a getblk request.  This is for convenience of
 * handling a partial last block.  Note that this is a relatively
 * expensive function for 64-bit binaries on platforms w/o native
 * 64-bit integers, so source->onlastblk is set to this value.
 * TODO: force source->blksize to a power of two? */
static inline
usize_t xd3_bytes_on_srcblk (xd3_source *source, xoff_t blkno)
{
  xoff_t s_1_div;
  usize_t s_1_rem;
  XD3_ASSERT (blkno < source->blocks);

  if (blkno != source->blocks - 1)
    {
      return source->blksize;
    }
  xd3_blksize_div(source->size - 1, source, &s_1_div, &s_1_rem);
  return s_1_rem + 1;
}

static inline
usize_t xd3_bytes_on_srcblk_fast (xd3_source *source, xoff_t blkno)
{
  return (blkno == source->blocks - 1 ?
	  source->onlastblk :
	  source->blksize);
}

#endif /* _XDELTA3_H_ */
