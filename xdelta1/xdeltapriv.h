/* -*- Mode: C;-*-
 *
 * This file is part of XDelta - A binary delta generator.
 *
 * Copyright (C) 1997, 1998, 1999  Josh MacDonald
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
 * $Id: xdeltapriv.h 1.29 Sat, 03 Apr 1999 20:09:14 -0800 jmacd $
 */

#ifndef _XDELTAPRIV_H_
#define _XDELTAPRIV_H_

#if 0
#define DEBUG_CKSUM_UPDATE
#define DEBUG_MATCH_PRINT
#define DEBUG_CKSUM
#define DEBUG_HASH
#define DEBUG_INST
#define CLOBBER_ALGORITHM_C
#define DEBUG_MD5
#define DEBUG_CONT
#define DEBUG_COPY
#define DEBUG_FIND
#define DEBUG_RSYNC_REQUEST
#define DEBUG_CONT
#define DEBUG_CONT2
#define DEBUG_CHECK_CONTROL
#endif

typedef struct _XdeltaPos         XdeltaPos;
typedef struct _RsyncHash         RsyncHash;

#define XPOS(p) (((p).page * (p).page_size) + (p).off)

struct _XdeltaPos {
  guint page;
  guint page_size;
  guint off;

  const guint8* mem;
  guint mem_page;
  guint mem_rem;
};

#define handle_length(x)    ((* (x)->table->table_handle_length) (x))
#define handle_pages(x)     ((* (x)->table->table_handle_pages) (x))
#define handle_pagesize(x)  ((* (x)->table->table_handle_pagesize) (x))
#define handle_map_page(x,y,z)  ((* (x)->table->table_handle_map_page) ((x),(y),(z)))
#define handle_unmap_page(x,y,z) ((* (x)->table->table_handle_unmap_page) ((x),(y),(z)))
#define handle_checksum_md5(x)   ((* (x)->table->table_handle_checksum_md5) (x))
#define handle_close(x,y)        ((* (x)->table->table_handle_close) ((x), (y)))
#define handle_write(x,y,z)      ((* (x)->table->table_handle_write) ((x),(y),(z)))
#define handle_copy(x,y,z,a)     ((* (x)->table->table_handle_copy) ((x),(y),(z),(a)))

struct _XdeltaGenerator
{
  GPtrArray *sources;

  const guint32 *table;
  guint          table_size;

  guint          to_output_pos;
  guint          data_output_pos;

  XdeltaOutStream  *data_out;
  XdeltaOutStream  *control_out;

  XdeltaControl    *control;

  XdeltaSource     *data_source;

#ifdef DEBUG_HASH
  gint hash_conflicts;           /* bucket already used. */
  gint hash_real_conflicts;      /* bucket had different checksum. */
  gint hash_real_real_conflicts; /* bucket had same checksum, different region */
  gint hash_fill;
  gint hash_entries;
#endif

  EdsioMD5Ctx ctx;
};

struct _XdeltaSource
{
  XdeltaStream    *source_in;
  XdeltaPos        source_pos;

  gint                   ck_count; /* number of elts in cksums. */
  const XdeltaChecksum  *cksums;   /* array of cksums. */

  const char *name;

  XdeltaStream    *index_in;
  XdeltaOutStream *index_out;

  gint             source_index;
  gboolean         used;
  gboolean         sequential;
  guint32          position;
};

#define CHEW(x) (single_hash[(guint)x])
#define FLIP_FORWARD(p)  if ((p).off == (p).page_size) { (p).page += 1; (p).off = 0; }

extern const guint16 single_hash[256];

void           init_pos               (XdeltaStream* str, XdeltaPos* pos);
gboolean       unmap_page             (XdeltaStream* stream, XdeltaPos* pos);
gboolean       map_page               (XdeltaStream* stream, XdeltaPos* pos);
gboolean       check_stream_integrity (XdeltaStream* str, const guint8* md5, guint len);
XdeltaControl* control_new            (void);

guint          c_hash                 (const XdeltaChecksum* c);

#endif /* _XDELTAPRIV_H_ */
