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
 * $Id: xdelta.c 1.4.1.50.1.2 Fri, 29 Jun 2001 06:01:08 -0700 jmacd $
 */

#define G_DISABLE_ASSERT

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "xdelta.h"
#include "xdeltapriv.h"

/* $Format: "const guint xdelta_major_version = $ReleaseMajorVersion$;" $ */
const guint xdelta_major_version = 1;
/* $Format: "const guint xdelta_minor_version = $ReleaseMinorVersion$;" $ */
const guint xdelta_minor_version = 1;
/* $Format: "const guint xdelta_micro_version = $ReleaseMicroVersion$;" $ */
const guint xdelta_micro_version = 3;

/* Control functions.
 */

static XdeltaControl* control_version_0 (SerialVersion0Control* cont) ;
static void           control_copy   (XdeltaControl* cont, XdeltaSource* src, guint from, guint to);
static gboolean       control_add_info (XdeltaControl* cont, XdeltaSource* src, const guint8* md5, guint len);

#ifndef XDELTA_HARDCODE_SIZE
int QUERY_SIZE = 0;
int QUERY_SIZE_POW = 0;
int QUERY_SIZE_MASK = 0;
#endif

int xdp_set_query_size_pow (int size_pow)
{
#ifdef XDELTA_HARDCODE_SIZE
  return XDP_QUERY_HARDCODED;
#else

  int x        = 1;
  int size_log = 0;

  for (; x != 0; x <<= 1, size_log += 1)
    {
      if (x == size_pow)
	goto good;
    }

  return XDP_QUERY_POW2;

 good:

  QUERY_SIZE      = size_log;
  QUERY_SIZE_POW  = size_pow;
  QUERY_SIZE_MASK = size_pow-1;

  return 0;

#endif
}

int
xdp_blocksize ()
{
  if (QUERY_SIZE == 0)
    {
      xdp_set_query_size_pow (1<<QUERY_SIZE_DEFAULT);
    }

  return QUERY_SIZE_POW;
}

const char*
xdp_errno (int errval)
{
  switch (errval)
    {
    case XDP_QUERY_HARDCODED:
      return "query size hardcoded";
    case XDP_QUERY_POW2:
      return "query size must be a power of 2";
    }

  return g_strerror (errval);
}

const guint16 single_hash[256] =
{
  /* Random numbers generated using SLIB's pseudo-random number
   * generator. */
  0xbcd1, 0xbb65, 0x42c2, 0xdffe, 0x9666, 0x431b, 0x8504, 0xeb46,
  0x6379, 0xd460, 0xcf14, 0x53cf, 0xdb51, 0xdb08, 0x12c8, 0xf602,
  0xe766, 0x2394, 0x250d, 0xdcbb, 0xa678, 0x02af, 0xa5c6, 0x7ea6,
  0xb645, 0xcb4d, 0xc44b, 0xe5dc, 0x9fe6, 0x5b5c, 0x35f5, 0x701a,
  0x220f, 0x6c38, 0x1a56, 0x4ca3, 0xffc6, 0xb152, 0x8d61, 0x7a58,
  0x9025, 0x8b3d, 0xbf0f, 0x95a3, 0xe5f4, 0xc127, 0x3bed, 0x320b,
  0xb7f3, 0x6054, 0x333c, 0xd383, 0x8154, 0x5242, 0x4e0d, 0x0a94,
  0x7028, 0x8689, 0x3a22, 0x0980, 0x1847, 0xb0f1, 0x9b5c, 0x4176,
  0xb858, 0xd542, 0x1f6c, 0x2497, 0x6a5a, 0x9fa9, 0x8c5a, 0x7743,
  0xa8a9, 0x9a02, 0x4918, 0x438c, 0xc388, 0x9e2b, 0x4cad, 0x01b6,
  0xab19, 0xf777, 0x365f, 0x1eb2, 0x091e, 0x7bf8, 0x7a8e, 0x5227,
  0xeab1, 0x2074, 0x4523, 0xe781, 0x01a3, 0x163d, 0x3b2e, 0x287d,
  0x5e7f, 0xa063, 0xb134, 0x8fae, 0x5e8e, 0xb7b7, 0x4548, 0x1f5a,
  0xfa56, 0x7a24, 0x900f, 0x42dc, 0xcc69, 0x02a0, 0x0b22, 0xdb31,
  0x71fe, 0x0c7d, 0x1732, 0x1159, 0xcb09, 0xe1d2, 0x1351, 0x52e9,
  0xf536, 0x5a4f, 0xc316, 0x6bf9, 0x8994, 0xb774, 0x5f3e, 0xf6d6,
  0x3a61, 0xf82c, 0xcc22, 0x9d06, 0x299c, 0x09e5, 0x1eec, 0x514f,
  0x8d53, 0xa650, 0x5c6e, 0xc577, 0x7958, 0x71ac, 0x8916, 0x9b4f,
  0x2c09, 0x5211, 0xf6d8, 0xcaaa, 0xf7ef, 0x287f, 0x7a94, 0xab49,
  0xfa2c, 0x7222, 0xe457, 0xd71a, 0x00c3, 0x1a76, 0xe98c, 0xc037,
  0x8208, 0x5c2d, 0xdfda, 0xe5f5, 0x0b45, 0x15ce, 0x8a7e, 0xfcad,
  0xaa2d, 0x4b5c, 0xd42e, 0xb251, 0x907e, 0x9a47, 0xc9a6, 0xd93f,
  0x085e, 0x35ce, 0xa153, 0x7e7b, 0x9f0b, 0x25aa, 0x5d9f, 0xc04d,
  0x8a0e, 0x2875, 0x4a1c, 0x295f, 0x1393, 0xf760, 0x9178, 0x0f5b,
  0xfa7d, 0x83b4, 0x2082, 0x721d, 0x6462, 0x0368, 0x67e2, 0x8624,
  0x194d, 0x22f6, 0x78fb, 0x6791, 0xb238, 0xb332, 0x7276, 0xf272,
  0x47ec, 0x4504, 0xa961, 0x9fc8, 0x3fdc, 0xb413, 0x007a, 0x0806,
  0x7458, 0x95c6, 0xccaa, 0x18d6, 0xe2ae, 0x1b06, 0xf3f6, 0x5050,
  0xc8e8, 0xf4ac, 0xc04c, 0xf41c, 0x992f, 0xae44, 0x5f1b, 0x1113,
  0x1738, 0xd9a8, 0x19ea, 0x2d33, 0x9698, 0x2fe9, 0x323f, 0xcde2,
  0x6d71, 0xe37d, 0xb697, 0x2c4f, 0x4373, 0x9102, 0x075d, 0x8e25,
  0x1672, 0xec28, 0x6acb, 0x86cc, 0x186e, 0x9414, 0xd674, 0xd1a5
};

/* Compute the hash of length len for buf, a single byte array of
 * values, by first indexing the random array.
 */

static void
init_query_checksum (const guint8 *buf, XdeltaChecksum *cksum)
{
  gint i       = QUERY_SIZE_POW;
  guint16 low  = 0;
  guint16 high = 0;

  for (; i != 0; i -= 1)
    {
      low  += CHEW(*buf++);
      high += low;
    }

  cksum->low  = low;
  cksum->high = high;
}

/* Generate checksums
 */

static gboolean
generate_checksums (XdeltaStream    *stream,
		    XdeltaSource    *source)
{
  gint total_checksums  = handle_length (stream) / QUERY_SIZE_POW;
  gint checksum_index   = 0;
  XdeltaChecksum cksum;
  XdeltaChecksum *result;
  const guint8* segment = NULL, *segment_pointer;
  gint   segment_len, orig_segment_len;
  guint  segment_page = 0;
  guint pages;

#ifdef DEBUG_CKSUM
  g_print ("Total base checksums: %d\n", total_checksums);
#endif

  source->ck_count = total_checksums;

  if (total_checksums == 0)
    return TRUE;

  /* This is the in-core FROM checksum table. */
  result         = g_new (XdeltaChecksum, total_checksums);
  source->cksums = result;

  for (pages = handle_pages (stream); segment_page <= pages; segment_page += 1)
    {
      segment_len = handle_map_page (stream, segment_page, &segment);

      if (segment_len < 0)
	return FALSE;

      orig_segment_len = segment_len;

      segment_len >>= QUERY_SIZE;

      for (segment_pointer = segment;
	   segment_len != 0;
	   segment_len -= 1, segment_pointer += QUERY_SIZE_POW)
	{
	  /* note: cheating at the boundaries */
	  init_query_checksum (segment_pointer, &cksum);

#ifdef DEBUG_CKSUM
	  g_print ("New cksum %04x %04x indices %d-%d\n",
		   cksum.low, cksum.high,
		   checksum_index * QUERY_SIZE_POW, (checksum_index * QUERY_SIZE_POW) + QUERY_SIZE_POW - 1);
#endif

	  result[checksum_index++] = cksum;
	}

      if (! handle_unmap_page (stream, segment_page, &segment))
	return FALSE;
    }

  return TRUE;
}

/* $Format: "#define XDELTA_REQUIRED_VERSION \"$ReleaseMajorVersion$.$ReleaseMinorVersion$.\"" $ */
#define XDELTA_REQUIRED_VERSION "1.1."

XdeltaGenerator*
__xdp_generator_new (const char* version)
{
  XdeltaGenerator* xg;

  if (strncmp (version, XDELTA_REQUIRED_VERSION, strlen (XDELTA_REQUIRED_VERSION)) != 0)
    g_error ("XDelta library version mismatched, compiled for %s, running %s\n", version, XDELTA_VERSION);

  xg = g_new0 (XdeltaGenerator, 1);

  xg->sources = g_ptr_array_new ();

  xg->data_source = g_new0 (XdeltaSource, 1);
  xg->data_source->name = "(patch data)";

  g_ptr_array_add (xg->sources, xg->data_source);

  return xg;
}

void
xdp_generator_free (XdeltaGenerator *gen)
{
  int i;

  for (i = 0; i < gen->sources->len; i += 1)
    xdp_source_free (gen->sources->pdata[i]);

  g_ptr_array_free (gen->sources, TRUE);
  g_free ((void*) gen->table);
  g_free (gen);
}

void
init_pos (XdeltaStream* str, XdeltaPos* pos)
{
  g_assert (str);

  memset (pos, 0, sizeof (*pos));

  pos->page_size = handle_pagesize (str);
}

gboolean
check_stream_integrity (XdeltaStream* str, const guint8* md5, guint len)
{
  const guint8* act_md5;

  if (len != handle_length (str))
    {
      xd_generate_handleintint_event (EC_XdStreamLengthFailed, str, len, handle_length (str));
      return FALSE;
    }

  act_md5 = handle_checksum_md5 (str);

  if (! act_md5)
    return FALSE;

  if (memcmp (md5, act_md5, 16) != 0)
    {
      char exp[33], rec[33];

      edsio_md5_to_string (md5, exp);
      edsio_md5_to_string (act_md5, rec);

      xd_generate_handlestringstring_event (EC_XdStreamChecksumFailed, str, exp, rec);
      g_free ((void*) act_md5);
      return FALSE;
    }

  g_free ((void*) act_md5);

  return TRUE;
}

static gboolean
xdp_source_index_read (XdeltaSource    *xs,
		       XdeltaStream    *index_in)
{
  SerialSource *ss = handle_source (index_in);
  XdeltaIndex *index;

  if (! ss)
    return FALSE;

  if (! unserialize_xdeltaindex (ss, &index))
    return FALSE;

  if (! check_stream_integrity (xs->source_in, index->file_md5, index->file_len))
    return FALSE;

  xs->ck_count = index->index_len;
  xs->cksums = index->index;

  /* @@@ how to free this? */

  return TRUE;
}

static gboolean
xdp_source_index_internal (XdeltaSource    *init,
			   XdeltaStream    *source_in,
			   XdeltaOutStream *index_out)
{
  if (! generate_checksums (source_in, init))
    return FALSE;

  if (index_out)
    {
      const guint8* source_in_md5;
      SerialSink* sink = handle_sink (index_out, NULL, NULL, NULL, NULL);

      if (! sink)
	return FALSE;

      if (! (source_in_md5 = handle_checksum_md5 (source_in)))
	return FALSE;

      if (! serialize_xdeltaindex (sink,
				   handle_length (source_in),
				   source_in_md5,
				   init->ck_count,
				   init->cksums))
	{
	  g_free ((void*) source_in_md5);
	  return FALSE;
	}

      g_free ((void*) source_in_md5);

      if (! handle_close (index_out, 0))
	return FALSE;
    }

  return TRUE;
}

gboolean
xdp_source_index (XdeltaStream    *source_in,
		  XdeltaOutStream *index_out)
{
  XdeltaSource* xs = xdp_source_new ("(ignored)", source_in, NULL, index_out);

  if (xs)
    {
      xdp_source_free (xs);
      return TRUE;
    }

  return FALSE;
}

XdeltaSource*
xdp_source_new (const char      *name,
		XdeltaStream    *source_in,
		XdeltaStream    *index_in,
		XdeltaOutStream *index_out)
{
  XdeltaSource* xs = g_new0 (XdeltaSource, 1);

  xs->source_in = source_in;
  xs->name = name;

  g_return_val_if_fail (source_in, NULL);
  g_return_val_if_fail (index_in ? ! index_out : TRUE, NULL);

  xs->index_in = index_in;
  xs->index_out = index_out;
  xs->source_pos.page_size = handle_pagesize (source_in);

  return xs;
}

static gboolean
xdp_source_check_index (XdeltaSource    *xs)
{
  if (xs->source_index == 0)
    return TRUE;

  if (! xs->index_in)
    return xdp_source_index_internal (xs, xs->source_in, xs->index_out);
  else
    return xdp_source_index_read (xs, xs->index_in);
}

void
xdp_source_free (XdeltaSource* xs)
{
  if (xs)
    {
      /* if (xs->ckarray) @@@ this is troublesome now
	g_free (xs->ckarray);*/

      g_free (xs);
    }
}

void
xdp_source_add (XdeltaGenerator *gen,
		XdeltaSource    *src)
{
  if (gen->table)
    {
      g_free ((gpointer)gen->table);
      gen->table = NULL;
    }

  g_ptr_array_add (gen->sources, src);
}

guint
c_hash (const XdeltaChecksum* c)
{
  const guint high = c->high;
  const guint low = c->low;
  const guint it = (high >> 2) + (low << 3) + (high << 16);

  return (it ^ high ^ low);
}

static gboolean
region_insert (XdeltaGenerator* gen, const XdeltaPos *xpos, guint len)
{
  /* This is a little bit cryptic: the xpos.mem + EXPR expression
   * computes the offset into the current page, which is guaranteed
   * to be correct since map_page has not occured yet. */
  const guint8* buf = xpos->mem + (gen->to_output_pos % xpos->page_size);

  if (len == 0)
    return TRUE;

#ifdef DEBUG_INST
  g_print ("insert %d at %d\n", len, gen->to_output_pos);
#endif

  if (! handle_write (gen->data_out, buf, len))
    return FALSE;

  control_copy (gen->control, gen->data_source, gen->data_output_pos, gen->data_output_pos + len);

  gen->to_output_pos += len;
  gen->data_output_pos += len;

  return TRUE;
}

static gboolean
region_copy (XdeltaGenerator* gen, XdeltaSource* src, guint from, guint to)
{
#ifdef DEBUG_INST
  g_print ("copy %d - %d (%d) to %d\n", from, to, to-from, gen->to_output_pos);
#endif

  control_copy (gen->control, src, from, to);

  gen->to_output_pos += (to-from);

  return TRUE;
}

gboolean
unmap_page (XdeltaStream* stream, XdeltaPos* pos)
{
  if (! pos->mem)
    return TRUE;

  if (handle_unmap_page (stream,
			 pos->mem_page,
			 &pos->mem))
    {
      pos->mem = NULL;
      return TRUE;
    }

  return FALSE;
}

gboolean
map_page (XdeltaStream* stream, XdeltaPos* pos)
{
  gint on_page;

  if (pos->mem && pos->mem_page == pos->page)
    return TRUE;

  if (pos->mem)
    {
      if (! handle_unmap_page (stream,
			       pos->mem_page,
			       &pos->mem))
	return FALSE;

      pos->mem = NULL;
    }

  pos->mem_page = pos->page;

  on_page = handle_map_page (stream,
			     pos->mem_page,
			     &pos->mem);

  if (on_page >= 0)
    {
      pos->mem_rem = on_page;
      return TRUE;
    }

  return FALSE;
}

static gboolean
try_match (XdeltaGenerator *gen,
	   XdeltaStream    *in,
	   XdeltaPos       *xpos_ptr,
	   XdeltaSource    *src,
	   guint            src_offset,
	   gboolean        *found)
{
  XdeltaPos xpos = *xpos_ptr;
  XdeltaPos ypos = src->source_pos;
  gint rem, remsave;
  gint match_forward  = 0;
  gint match_backward = 0;
  gint match_forward_max;
  gint match_backward_max;
  guint to_offset = XPOS (xpos);
  gboolean one_insert = FALSE;

  *found = FALSE;

  ypos.page = src_offset / ypos.page_size;
  ypos.off  = src_offset % ypos.page_size;

  match_forward_max  = MIN (handle_length (in)             - to_offset,
			    handle_length (src->source_in) - src_offset);
  match_backward_max = MIN (src_offset, to_offset - gen->to_output_pos);

  /* Don't allow backward paging */
  match_backward_max = MIN (match_backward_max, xpos.off);

  /* We're testing against the negative below. */
  match_backward_max = - match_backward_max;

  for (; match_backward > match_backward_max; )
    {
      g_assert (xpos.off != 0);

      if (ypos.off == 0)
	{
	  ypos.off = ypos.page_size;
	  ypos.page -= 1;
	}

      if (! map_page (src->source_in, &ypos))
	goto bail;

      rem = MIN (xpos.off, ypos.off);
      rem = MIN (match_backward - match_backward_max, rem);

      for (; rem > 0; rem -= 1, match_backward -= 1)
	{
	  if (xpos.mem[xpos.off-1] != ypos.mem[ypos.off-1])
	    goto doneback;

	  xpos.off -= 1;
	  ypos.off -= 1;
	}
    }

doneback:

  xpos.page = to_offset / xpos.page_size;
  xpos.off  = to_offset % xpos.page_size;

  ypos.page = src_offset / ypos.page_size;
  ypos.off  = src_offset % ypos.page_size;

  for (; match_forward < match_forward_max; )
    {
      if (! map_page (src->source_in, &ypos))
	goto bail;

      /* Fortunately, if this map happens it means that the match must
       * be long enough to succeed below, therefore it is safe to write
       * the insert out now. */
      if (! one_insert && xpos.page != xpos.mem_page)
	{
	  one_insert = TRUE;

	  if (! region_insert (gen, &xpos, (to_offset + match_backward) - gen->to_output_pos))
	    goto bail;
	}

      if (! map_page (in, &xpos))
	goto bail;

      rem = MIN (xpos.mem_rem - xpos.off, ypos.mem_rem - ypos.off);
      rem = MIN (match_forward_max - match_forward, rem);

      /* Do a int-wise comparison if the regions are aligned. */
      if (rem > (4*sizeof(int)) && (xpos.off % sizeof (int)) == (ypos.off % sizeof(int)))
	{
	  gint is;
	  const int *xi, *yi;

	  for (; xpos.off % sizeof(int); rem -= 1, match_forward += 1)
	    {
	      if (xpos.mem[xpos.off] != ypos.mem[ypos.off])
		goto done;

	      xpos.off += 1;
	      ypos.off += 1;
	    }

	  remsave = rem;
	  rem /= sizeof(int);

	  xi = (const int*) (xpos.mem + xpos.off);
	  yi = (const int*) (ypos.mem + ypos.off);

	  is = rem;

	  for (; rem > 0 && *xi == *yi; )
	    {
	      rem -= 1;
	      xi += 1;
	      yi += 1;
	    }

	  is -= rem;

	  match_forward += is * sizeof(int);
	  xpos.off      += is * sizeof(int);
	  ypos.off      += is * sizeof(int);

 	  rem = (rem * sizeof(int)) + (remsave % sizeof(int));
	}

      for (; rem > 0; rem -= 1, match_forward += 1)
	{
	  if (xpos.mem[xpos.off] != ypos.mem[ypos.off])
	    goto done;

	  xpos.off += 1;
	  ypos.off += 1;
	}

      FLIP_FORWARD (xpos);
      FLIP_FORWARD (ypos);
    }

done:

  if (match_forward - match_backward >= QUERY_SIZE_POW)
    {
      *found = TRUE;

      if (! one_insert)
	{
	  if (! region_insert (gen, &xpos, (to_offset + match_backward) - gen->to_output_pos))
	    goto bail;
	}

      if (! region_copy (gen, src, src_offset + match_backward, src_offset + match_forward))
	goto bail;
    }
  else
    {
      g_assert (! one_insert);
    }

  *xpos_ptr = xpos;
  src->source_pos = ypos;
  return TRUE;

bail:
  *xpos_ptr = xpos;
  src->source_pos = ypos;
  return FALSE;
}

static gboolean
compute_copies (XdeltaGenerator* gen, XdeltaStream* stream)
{
  XdeltaChecksum cksum;
  const XdeltaChecksum *source_cksum;
  const guint8 *segment_pointer;
  guint source_offset, segment_index, index, prime = gen->table_size;
  guint source_index;
  const guint32* table = gen->table;
  guint16 old_c, new_c;
  guint save_page, save_off;
#ifdef DEBUG_MATCH_PRINT
  guint i;
#endif
  XdeltaPos xpos;
  gboolean found;
  gboolean ret = TRUE;

  if (handle_length (stream) < QUERY_SIZE_POW)
    return TRUE;

  init_pos (stream, &xpos);

  while (XPOS (xpos) <= (handle_length (stream) - QUERY_SIZE_POW))
    {
      if (!map_page (stream, &xpos))
	return FALSE;

      g_assert (xpos.mem_rem > xpos.off);

      segment_index = (xpos.mem_rem - xpos.off);

      if (segment_index < QUERY_SIZE_POW)
	goto nextpage;

      segment_index -= QUERY_SIZE_POW;

      segment_pointer = xpos.mem + xpos.off;

      init_query_checksum (segment_pointer, &cksum);

      for (; ; segment_pointer += 1)
	{
#ifdef DEBUG_CKSUM_UPDATE
	  XdeltaChecksum cktest;

	  init_query_checksum (segment_pointer, &cktest);

	  if (cktest.high != cksum.high || cktest.low != cktest.low)
	    abort ();
#endif

	  index = c_hash (&cksum) % prime;

#ifdef DEBUG_MATCH_PRINT
	  g_print ("%d: searching for match \"", XPOS(xpos));
	  for (i = 0; i < QUERY_SIZE_POW; i += 1)
	    {
	      if (isprint (segment_pointer[i]))
		g_print ("%c", segment_pointer[i]);
	      else
		g_print ("\\0%o", segment_pointer[i]);
	    }
	  g_print ("\"... %s\n", table[index] ? "found" : "notfound");
#endif

	  if (table[index])
	    {
	      source_index  = (table[index] &  QUERY_SIZE_MASK) - 1;
	      source_offset = (table[index] >> QUERY_SIZE);

	      source_cksum = ((XdeltaSource*)gen->sources->pdata[source_index])->cksums + source_offset;

	      if (cksum.high == source_cksum->high &&
		  cksum.low  == source_cksum->low)
		{
		  save_page = xpos.page;
		  save_off  = xpos.off;

		  if (! try_match (gen,
				   stream,
				   &xpos,
				   gen->sources->pdata[source_index],
				   source_offset << QUERY_SIZE,
				   &found))
		    {
		      ret = FALSE;
		      goto bail;
		    }

		  if (found)
		    {
		      g_assert (xpos.page*xpos.page_size+xpos.off == gen->to_output_pos);

		      goto reenter;
		    }
		  else
		    {
		      xpos.page = save_page;
		      xpos.off  = save_off;
		    }
		}
	    }

	  if (segment_index == 0)
	    goto nextpage;

	  segment_index -= 1;
	  xpos.off += 1;

	  old_c = CHEW(segment_pointer[0]);
	  new_c = CHEW(segment_pointer[QUERY_SIZE_POW]);

	  cksum.low -= old_c;
	  cksum.low += new_c;

	  cksum.high -= old_c << QUERY_SIZE;
	  cksum.high += cksum.low;
	}

    nextpage:

      if (xpos.mem_rem < xpos.page_size)
	break;

      xpos.page += 1;
      xpos.off = 0;

      if (xpos.page != xpos.mem_page)
	{
	  if (! region_insert (gen, &xpos, XPOS (xpos) - gen->to_output_pos))
	    return FALSE;
	}

    reenter:
      (void) 0;
    }

  xpos.off = gen->to_output_pos % handle_pagesize (stream);

  while (gen->to_output_pos < handle_length (stream))
    {
      if (! map_page (stream, &xpos))
	return FALSE;

      if (! region_insert (gen, &xpos, xpos.mem_rem - xpos.off))
	ret = FALSE;

      xpos.off = 0;
      xpos.page += 1;
    }

bail:

  if (! unmap_page (stream, &xpos))
      return FALSE;

  return ret;
}

static gboolean
just_output (XdeltaGenerator *gen,
	     XdeltaStream    *in)
{
  XdeltaPos pos;

  init_pos (in, &pos);

  while (gen->to_output_pos < handle_length (in))
    {
      if (! map_page (in, &pos))
	return FALSE;

      if (! region_insert (gen, &pos, pos.mem_rem - pos.off))
	return FALSE;

      pos.off = 0;
      pos.page += 1;
    }

  if (! unmap_page (in, &pos))
    return FALSE;

  return TRUE;
}

/* Clobbering decision (see below):
 *
 * Algorithm A: Clobber it always (its fast!).  The problem
 *              is that this prefers matches at the front of
 *              the file and leads to poor matches at the back
 *              of the file (assuming I insert going backwards).
 *
 * Algorithm B: Keep a table of how many times there has
 *              been a clobber at each index i, C[i].
 *              With probability 1/(C[i]+1), replace the
 *              previous entry.  This gives a uniform
 *              probability of each entry surviving.
 *              The problem (supposed) with this
 *              algorithm is that the probabilities
 *              should not be uniform (though uniform is
 *              better than A) because there are more
 *              chances to match a segment at the end of
 *              the file than at the beginning.
 *
 * Algorithm C: Give a linear weight to each match
 *              according to it's position in the file
 *              -- number the segments from N down to 1
 *              starting at the beginning.  Same as the
 *              above but with a different weight.  The
 *              weight for segment i, match at checksum
 *              offset k, follows.  The total number of
 *              checksums in the segment is C_i,
 *              therefore the total checksum count is
 *              C = sum_i (C_i).
 *              Now the interior weight is the (C_i-k)
 *              (the linear assumption) and the total
 *              interior weight is sum_{j=1}^{N}{j}=(N)(N+1)/2
 *              so the kth segment has interior weight
 *
 *                [2 (C_i - k)] / [(C_i) (C_i + 1)]
 *
 *              add in the exterior weight (after
 *              cancelling a C_i):
 *
 *                w(i,k) = [2 (C_i - k)] / [(C_i + 1) (C)]
 *
 *              Now, as above, we will compute whether to
 *              keep or replace the current value at the j-th
 *              decision.  Let R_j be the running sum of
 *              weights considered so far.  R_0 = 0.  At the
 *              j-th decision,
 *
 *                P_ikj(use new value) = w(i,k)/R_{j+1}
 *                R_{j+1} = R_j + w(i,k)
 */

static gboolean
xdp_generate_delta_int (XdeltaGenerator *gen,
			XdeltaStream    *in,
			XdeltaOutStream *control_out,
			XdeltaOutStream *data_out)
{
  gint i, j, total_from_ck_count = 0, prime = 0, index = 0;
  gint total_from_len = 0;
  guint32* table = NULL;

  if (QUERY_SIZE == 0)
    {
      xdp_set_query_size_pow (1<<QUERY_SIZE_DEFAULT);
    }

  if (gen->sources->len == 0)
    {
      xd_generate_void_event (EC_XdTooFewSources);
      return FALSE;
    }

  for (i = 0; i < gen->sources->len; i += 1)
    {
      XdeltaSource* src = gen->sources->pdata[i];

      src->used = FALSE;
      src->sequential = TRUE;
      src->position = 0;
      src->source_index = i;

      if (src->source_index != 0)
	total_from_len += handle_length (src->source_in);
    }

  /* QUERY_SIZE_POW is the number of elements freed in the cksum hash
   * table for storing segment number + (offset/QUERY_SIZE_POW) in.  1
   * for the zero + 1 for the data segment */
  if (gen->sources->len > (QUERY_SIZE_POW-2))
    {
      xd_generate_void_event (EC_XdTooManySources);
      return FALSE;
    }

  if (handle_length (in) < QUERY_SIZE_POW || total_from_len < QUERY_SIZE_POW)
    {
      if (! just_output (gen, in))
	return FALSE;
    }
  else
    {
      for (i = 0; i < gen->sources->len; i += 1)
	{
	  XdeltaSource* xs = (XdeltaSource*)gen->sources->pdata[i];

	  if (! xdp_source_check_index (xs))
	    return FALSE;

	  total_from_ck_count += xs->ck_count;
	}

      prime = g_spaced_primes_closest (total_from_ck_count);

      gen->table = table = g_new0 (guint32, prime);
      gen->table_size = prime;

      for (i = 0; i < gen->sources->len; i += 1)
	{
	  XdeltaSource* xs = (XdeltaSource*)gen->sources->pdata[i];

	  for (j = xs->ck_count-1; j >= 0; j -= 1)
	    {
	      index = c_hash (xs->cksums + j) % prime;

#ifdef DEBUG_HASH
	      gen->hash_entries += 1;

	      if (table[index])
		{
		  gen->hash_conflicts += 1;

		  /*regions_similar (gen,
				   i,
				   j,
				   (table[index] & QUERY_SIZE_MASK) - 1,
				   table[index] >> QUERY_SIZE);*/
		}
#endif

	      /* This is the real code */
	      table[index] = (j << QUERY_SIZE) + 1 + i;
	    }
	}

#ifdef DEBUG_HASH
      for (i = 0; i < prime; i += 1)
	{
	  if (gen->table[i])
	    gen->hash_fill += 1;
	}

      g_print ("*** Hash stats:\n");
      g_print ("Hash conflicts: %d\n", gen->hash_conflicts);
      g_print ("Hash real conflicts: %d\n", gen->hash_real_conflicts);
      g_print ("Hash real real conflicts: %d\n", gen->hash_real_real_conflicts);
      g_print ("Hash fill:      %d\n", gen->hash_fill);
      g_print ("Hash size:      %d\n", gen->table_size);
      g_print ("Hash entries:   %d\n", gen->hash_entries);
      g_print ("Hash fill/entries: %f\n", (float)gen->hash_fill/(float)gen->hash_entries);
#endif

      if (! compute_copies (gen, in))
	return FALSE;
    }

  return TRUE;
}

XdeltaControl*
xdp_generate_delta (XdeltaGenerator *gen,
		    XdeltaStream    *in,
		    XdeltaOutStream *control_out,
		    XdeltaOutStream *data_out)
{
  gint i;
  const guint8* in_md5;
  const guint8* data_out_md5;

  gen->data_out = data_out;
  gen->control_out = control_out;
  gen->control = control_new ();

  if (! xdp_generate_delta_int (gen, in, control_out, data_out))
    return NULL;

  if (! handle_close (data_out, 0))
    return NULL;

  if (! (in_md5 = handle_checksum_md5 (in)))
    return FALSE;

  if (! (data_out_md5 = handle_checksum_md5 (data_out)))
    return FALSE;

  gen->control->has_data = gen->data_source->used;
  gen->control->inst = &g_array_index (gen->control->inst_array, XdeltaInstruction, 0);
  gen->control->inst_len = gen->control->inst_array->len;

  gen->control->to_len = handle_length (in);
  memcpy (gen->control->to_md5, in_md5, 16);

  for (i = 0; i < gen->sources->len; i += 1)
    {
      XdeltaSource* src = gen->sources->pdata[i];
      const guint8* md5;
      guint len;

      if (src->source_in)
	{
	  if (! (md5 = handle_checksum_md5 (src->source_in)))
	    return FALSE;

	  len = handle_length (src->source_in);
	}
      else
	{
	  len = handle_length (data_out);
	  md5 = data_out_md5;
	}

      if (! control_add_info (gen->control, src, md5, len))
	return NULL;
    }

  gen->control->source_info = (XdeltaSourceInfo**) gen->control->source_info_array->pdata;
  gen->control->source_info_len = gen->control->source_info_array->len;

  if (control_out && ! xdp_control_write (gen->control, control_out))
    return NULL;

  return gen->control;
}

/* Below here boring details mostly to do with reading and writing
 * control. */

XdeltaControl*
control_new (void)
{
  XdeltaControl* it = g_new0 (XdeltaControl, 1);

  it->inst_array        = g_array_new (FALSE, FALSE, sizeof (XdeltaInstruction));
  it->source_info_array = g_ptr_array_new ();

  return it;
}

static void
control_reindex (XdeltaControl* cont, XdeltaSource* src)
{
  gint i;
  gint new_index = cont->source_info_array->len;

  for (i = 0; i < cont->inst_len; i += 1)
    {
      XdeltaInstruction* inst = cont->inst + i;

      if (inst->index == src->source_index)
	inst->index = new_index;
    }
}

gboolean
control_add_info (XdeltaControl* cont, XdeltaSource* src, const guint8* md5, guint len)
{
  XdeltaSourceInfo* si;

  if (! src->used)
    return TRUE;

  si = g_new0 (XdeltaSourceInfo, 1);

  si->name = src->name;
  si->sequential = src->sequential;
  si->len = len;
  si->isdata = (src->source_index == 0);

  memcpy (si->md5, md5, 16);

  control_reindex (cont, src);

  g_ptr_array_add (cont->source_info_array, si);

  return TRUE;
}

void
xdp_control_free (XdeltaControl* cont)
{
  if (cont->source_info_array)
    g_ptr_array_free (cont->source_info_array, TRUE);
  if (cont->inst_array)
    g_array_free (cont->inst_array, TRUE);
  g_free (cont);
}

void
control_copy (XdeltaControl* cont, XdeltaSource* src, guint from, guint to)
{
  XdeltaInstruction i;

  if (cont->inst_array->len > 0)
    {
      XdeltaInstruction* oi = & g_array_index (cont->inst_array, XdeltaInstruction, cont->inst_array->len-1);

      if (oi->index == src->source_index && (oi->offset + oi->length) == from)
	{
	  oi->length += (to - from);
	  return;
	}
    }

  i.index = src->source_index;
  i.offset = from;
  i.length = to - from;

  src->used = TRUE;

  if (src->position != from)
    src->sequential = FALSE;

  src->position = to;

  g_array_append_val (cont->inst_array, i);
}

#ifdef DEBUG_CONT
static void
print_info (XdeltaSourceInfo* si)
{
  char md5str[33];

  edsio_md5_to_string (si->md5, md5str);

  g_print (" ** info\n");
  g_print (" md5: %s\n", md5str);
  g_print (" len: %d\n", si->length);
}

static void
print_inst (XdeltaInstruction* i)
{
  switch (i->type)
    {
    case INST_TYPE_COPY:
      g_print ("    copy (%c) %d-%d (%d) from %d\n", i->type, i->offset, i->offset + i->length, i->length, i->index);
      break;
    case INST_TYPE_INSERT:
      g_print ("    insert %d\n", i->length);
      break;
    }
}

static void
xdp_print_control (XdeltaControl *cont)
{
  gint i;

  g_print ("*** control\n");

  g_print (" data len: %d\n", cont->data_len);
  print_info (&cont->to_info);

  g_print (" source info len: %d\n", cont->source_info_len);

  for (i = 0; i < cont->source_info_len; i += 1)
    print_info (cont->source_info[i]);

  g_print ("   inst len: %d\n", cont->inst_len);

  for (i = 0; i < cont->inst_len; i += 1)
    print_inst (cont->inst + i);

}
#endif

#ifdef DEBUG_CHECK_CONTROL
void
check_control (XdeltaControl* cont)
{
  gint i;

  for (i = 0; i < cont->inst_len; i += 1)
    {
      switch (cont->inst[i].type)
	{
	case INST_TYPE_NCOPY:
	case INST_TYPE_COPY:
	  if (cont->inst[i].index >= cont->source_info_len)
	    g_error ("control has a too high instruction index\n");
	}
    }
}
#endif

static gboolean
unpack_instructions (XdeltaControl* cont)
{
  gint i;
  guint output_pos = 0;

  for (i = 0; i < cont->source_info_len; i += 1)
    {
      XdeltaSourceInfo* info = cont->source_info[i];

      info->position = 0;
      info->copies = 0;
      info->copy_length = 0;
    }

  for (i = 0; i < cont->inst_len; i += 1)
    {
      XdeltaSourceInfo* info;
      XdeltaInstruction *inst = cont->inst + i;

      if (inst->index >= cont->source_info_len)
	{
	  xd_generate_int_event (EC_XdOutOfRangeSourceIndex, inst->index);
	  return FALSE;
	}

      info = cont->source_info[inst->index];

      if (info->sequential)
	{
	  inst->offset = info->position;
	  info->position = inst->offset + inst->length;
	}

      inst->output_start = output_pos;
      output_pos += inst->length;

      info->copies += 1;
      info->copy_length += inst->length;
    }

  return TRUE;
}

static gboolean
pack_instructions (XdeltaControl* cont)
{
  gint i;

  for (i = 0; i < cont->source_info_len; i += 1)
    {
      XdeltaSourceInfo* info = cont->source_info[i];

      info->position = 0;
      info->copies = 0;
      info->copy_length = 0;
    }

  for (i = 0; i < cont->inst_len; i += 1)
    {
      XdeltaSourceInfo* info;
      XdeltaInstruction *inst = cont->inst + i;

      if (inst->index >= cont->source_info_len)
	{
	  xd_generate_int_event (EC_XdOutOfRangeSourceIndex, inst->index);
	  return FALSE;
	}

      info = cont->source_info[inst->index];

      if (info->sequential)
	{
	  g_assert (info->position == inst->offset);
	  info->position += inst->length;
	  inst->offset = 0;
	}

      info->copies += 1;
      info->copy_length += inst->length;
    }

  return TRUE;
}


gboolean
xdp_control_write (XdeltaControl   *cont,
		   XdeltaOutStream *cont_out)
{
  SerialSink* sink = handle_sink (cont_out, NULL, NULL, NULL, NULL);

#ifdef DEBUG_CONT
  xdp_print_control (cont);
#endif
#ifdef DEBUG_CHECK_CONTROL
  check_control (cont);
#endif

  if (! sink)
    return FALSE;

  if (! pack_instructions (cont))
    return FALSE;

  /* @@@ think about how the count function overcounts on the instruction
   * array by a factor of 2 or more. */
  if (! serialize_xdeltacontrol_obj (sink, cont))
    return FALSE;

  if (! handle_close (cont_out, 0))
    return FALSE;

  return TRUE;
}

XdeltaControl*
xdp_control_read (XdeltaStream    *cont_in)
{
  SerialSource* src = handle_source (cont_in);
  XdeltaControl* cont;
  SerialType type;

  if (! src)
    return NULL;

  if (! serializeio_unserialize_generic_acceptable (src, ST_XdeltaControl | ST_Version0Control, & type, (void**) & cont))
    return NULL;

  if (type == ST_Version0Control)
    {
      SerialVersion0Control *ocont = (SerialVersion0Control*) cont;

      xd_generate_string_event (EC_XdBackwardCompatibilityMode, "1.0");

      cont = control_version_0 (ocont);

      g_free (ocont);
    }

  if (! unpack_instructions (cont))
    return NULL;

#ifdef DEBUG_CHECK_CONTROL
  check_control (cont);
#endif

  return cont;
}

XdeltaControl*
control_version_0 (SerialVersion0Control* ocont)
{
  XdeltaControl* cont = g_new0 (XdeltaControl, 1);
  gint i;
  XdeltaSourceInfo* dinfo;

  g_assert (! ocont->normalized);

  memcpy (cont->to_md5, ocont->to_info.real_md5, 16);

  cont->to_len = ocont->to_info.length;

  cont->has_data = TRUE;

  cont->source_info_len = ocont->source_info_len + 1;
  cont->source_info = g_new (XdeltaSourceInfo*, cont->source_info_len);

  cont->source_info[0] = dinfo = g_new0 (XdeltaSourceInfo, 1);

  dinfo->name = "(patch data)";
  memcpy (dinfo->md5, ocont->to_info.md5, 15);
  dinfo->len = ocont->data_len;
  dinfo->isdata = TRUE;
  dinfo->sequential = FALSE;

  for (i = 0; i < ocont->source_info_len; i += 1)
    {
      XdeltaSourceInfo* info = g_new0 (XdeltaSourceInfo, 1);
      SerialVersion0SourceInfo* oinfo = ocont->source_info[i];

      cont->source_info[i+1] = info;

      info->name = "unnamed";
      memcpy (info->md5, oinfo->md5, 16);
      info->len = oinfo->length;
      info->isdata = FALSE;
      info->sequential = FALSE;
    }

  /* The old unpack */
#define OLD_QUERY_SIZE 4
  for (i = 0; i < ocont->inst_len; i += 1)
    {
      switch (ocont->inst[i].length & 3)
	{
	case 0: ocont->inst[i].type = 'N'; break;
	case 1: ocont->inst[i].type = 'E'; break;
	case 2: ocont->inst[i].type = 'C'; break;
	case 3: ocont->inst[i].type = 'I'; break;
	}

      ocont->inst[i].length >>= 2;
      ocont->inst[i].index = ocont->inst[i].length & OLD_QUERY_SIZE;
      ocont->inst[i].length >>= OLD_QUERY_SIZE;
    }

  cont->inst_len = ocont->inst_len;
  cont->inst = g_new (XdeltaInstruction, cont->inst_len);

  for (i = 0; i < cont->inst_len; i += 1)
    {
      cont->inst[i].length = ocont->inst[i].length;
      cont->inst[i].offset = ocont->inst[i].offset;

      switch (ocont->inst[i].type)
	{
	case 'N':
	case 'E':
	  abort ();
	  break;

	case 'C':
	  g_assert (ocont->inst[i].index == 0);

	  cont->inst[i].index = 1;
	  break;

	case 'I':
	  cont->inst[i].index = 0;
	  break;
	}

    }

  return cont;
}
