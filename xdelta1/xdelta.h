/* -*- Mode: C;-*-
 *
 * This file is part of XDelta - A binary delta generator.
 *
 * Copyright (C) 1997, 1998, 2001  Josh MacDonald
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
 * $Id: xdelta.h 1.4.1.8.1.50.1.3 Fri, 29 Jun 2001 06:01:08 -0700 jmacd $
 */

#ifndef _XDELTA_H_
#define _XDELTA_H_

#include "xd_edsio.h"

typedef SerialRsyncIndex          XdeltaRsync;
typedef SerialRsyncIndexElt       XdeltaRsyncElt;
typedef SerialXdeltaChecksum      XdeltaChecksum;
typedef SerialXdeltaIndex         XdeltaIndex;
typedef SerialXdeltaSourceInfo    XdeltaSourceInfo;
typedef SerialXdeltaControl       XdeltaControl;
typedef SerialXdeltaInstruction   XdeltaInstruction;

typedef struct _XdeltaGenerator   XdeltaGenerator;
typedef struct _XdeltaSource      XdeltaSource;

typedef FileHandle XdeltaStream;
typedef FileHandle XdeltaOutStream;

/* Note: FileHandle is an opaque type, you must define it
 * to use this library.  See how its done in xdmain.c.
 */

/* $Format: "#define XDELTA_VERSION \"$ReleaseVersion$\"" $ */
#define XDELTA_VERSION "1.1.4"

/* $Format: "#define XDELTA_MAJOR_VERSION $ReleaseMajorVersion$" $ */
#define XDELTA_MAJOR_VERSION 1

/* $Format: "#define XDELTA_MINOR_VERSION $ReleaseMinorVersion$" $ */
#define XDELTA_MINOR_VERSION 1

/* $Format: "#define XDELTA_MICRO_VERSION $ReleaseMicroVersion$" $ */
#define XDELTA_MICRO_VERSION 4

extern const guint xdelta_major_version;
extern const guint xdelta_minor_version;
extern const guint xdelta_micro_version;

/* copy segments are of length 1<<QUERY_SIZE, this must not be greater
 * than 6 due to a space assumption, and also limits the number of
 * sources allowed to (QUERY_SIZE_POW-1).
 *
 * The in-core FROM CKSUM table's size is (FROM_LEN bytes * 4 bytes-per-checksum) / (1<<QUERY_SIZE)
 *
 * The in-core CKSUM HASH table's size is the same size, each checksum has one 32-bit offset+srcindex
 *
 * With the a value of (QUERY_SIZE = 4) gives a 16 byte block size,
 * gives FROM_LEN/4 bytes for the FROM CKSUM map, and the same for the
 * CKSUM HASH, giving FROM_LEN/2 bytes, in addition to whatever used
 * to cache the FROM inputs.
 **/
#define QUERY_SIZE_DEFAULT  4

/* The query size has historically been hard coded.  This gains around
 * 20% in speed, so I've left it the default.  If you're interested in
 * large files, try undefining this (and using the -s argument to
 * Xdelta).
 */
#define XDELTA_HARDCODE_SIZE

#ifdef XDELTA_HARDCODE_SIZE
#define QUERY_SIZE          QUERY_SIZE_DEFAULT
#define QUERY_SIZE_POW      (1<<QUERY_SIZE)
#define QUERY_SIZE_MASK     (QUERY_SIZE_POW-1)
#else
extern int QUERY_SIZE;
extern int QUERY_SIZE_POW;
extern int QUERY_SIZE_MASK;
#endif

#define XDP_QUERY_HARDCODED -7654
#define XDP_QUERY_POW2      -7655

/* Returns  if query size is hard coded. */
int xdp_set_query_size_pow (int size_pow);
int xdp_blocksize          ();

/* An xdelta consists of two pieces of information, the control and
 * data segments.  The control segment consists of instructions,
 * metadata, and some redundent information for validation.  The data
 * segment consists of literal data not found in any of the sources.
 *
 * The library operates on two types of streams, random access and
 * non-seekable.  Briefly, you initialize a XdeltaGenerator with one
 * or more XdeltaSources.  These XdeltaSources contain a random access
 * stream (a FROM file).
 *
 * The generator is initialized with an input stream and an output
 * stream, these streams are not seekable.
 *
 * The creation of a XdeltaSource requires a complete pass through the
 * file.  This pass pre-computes an index which is used during delta
 * computation.  This index may be saved and restored to avoid
 * computing it multiple times.
 *
 */

#define xdp_generator_new() __xdp_generator_new (XDELTA_VERSION)

XdeltaGenerator* __xdp_generator_new      (const char      *version);

/* Create a new source.  If non-null, INDEX_IN indicates that the
 * index was previously computed and may be read from the stream.  If
 * non-null, INDEX_OUT is a stream to which the the computed index
 * will be written.  INDEX_OUT is ignored when INDEX_IN is non-null.
 * Returns the source on success, NULL on failure.
 */

XdeltaSource*    xdp_source_new           (const char      *name,
					   XdeltaStream    *source_in,
					   XdeltaStream    *index_in,
					   XdeltaOutStream *index_out);

/* Simply index the source, do not save it in memory.  Returns true on
 * success, false on failure. */
gboolean         xdp_source_index         (XdeltaStream    *source_in,
					   XdeltaOutStream *index_out);

/* Add SRC to the generator.  The source will then be used for generating
 * deltas. */
void             xdp_source_add           (XdeltaGenerator *gen,
					   XdeltaSource    *src);

/* Actually generate a delta against the accumulated sources in GEN.
 * Returns the delta's controller or NULL on failure. */
XdeltaControl*   xdp_generate_delta       (XdeltaGenerator *gen,
					   XdeltaStream    *in,
					   XdeltaOutStream *control_out,
					   XdeltaOutStream *data_out);

/* Reads a control object from a stream. */
XdeltaControl*   xdp_control_read         (XdeltaStream    *cont_in);

/* Writes a control object to a stream. */
gboolean         xdp_control_write        (XdeltaControl   *cont,
					   XdeltaOutStream *cont_out);

/* Free the above structures */
void             xdp_source_free          (XdeltaSource    *src);
void             xdp_generator_free       (XdeltaGenerator *gen);
void             xdp_control_free         (XdeltaControl   *con);

/* Apply: (simple form) first set the IN field of each
 * XdeltaSourceInfo in the control, then call this. */

gboolean         xdp_apply_delta          (XdeltaControl     *cont,
					   XdeltaOutStream   *res);

/* Rsync: Undocumented, experimental code.  Have a look.  May not
 * compile. */

XdeltaRsync*     xdp_rsync_index          (XdeltaStream      *file,
					   guint              seg_len,
					   XdeltaStream      *cache_in,
					   XdeltaOutStream   *cache_out);

void             xdp_rsync_index_free     (XdeltaRsync       *rsync);

GArray*          xdp_rsync_request        (XdeltaStream      *file,
					   XdeltaRsync       *rsync);

gboolean         xdp_apply_rsync_reply    (XdeltaRsync       *rsync,
					   XdeltaStream      *from,
					   XdeltaStream      *reply,
					   XdeltaStream      *out);

const char* xdp_errno (int errval);

#endif
