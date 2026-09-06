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
 * $Id: xdrsync.c 1.2 Thu, 01 Apr 1999 23:29:11 -0800 jmacd $
 */

#include <string.h>
#include <stdlib.h>

#include "xdelta.h"
#include "xdeltapriv.h"

/* Rsync
 */

static void
init_long_checksum (const guint8 *buf, guint len, XdeltaChecksum *cksum)
{
  guint16 low  = cksum->low;
  guint16 high = cksum->high;

  /* @@@ unroll me? */
  for (; len > 0; len -= 1)
    {
      low  += CHEW(*buf++);
      high += low;
    }

  cksum->low  = low;
  cksum->high = high;
}

static XdeltaRsync*
xdp_rsync_index_int (XdeltaStream      *str,
		     guint              seg_len)
{
  guint to_index = seg_len;
  XdeltaPos pos;
  XdeltaChecksum cksum;
  GArray    *index;
  EdsioMD5Ctx ctx;

  index = g_array_new (FALSE, FALSE, sizeof (XdeltaRsyncElt));

  init_pos (str, &pos);

  memset (&cksum, 0, sizeof (cksum));

  edsio_md5_init (& ctx);

  for (;;)
    {
      gint consume;

      if (! map_page (str, &pos))
	return NULL;

      consume = MIN (to_index, pos.mem_rem - pos.off);

      if (consume == 0)
	break;

      to_index -= consume;

      edsio_md5_update (& ctx, pos.mem + pos.off, consume);
      init_long_checksum (pos.mem + pos.off, consume, &cksum);

      if (to_index == 0)
	{
	  XdeltaRsyncElt elt;

	  edsio_md5_final (elt.md5, &ctx);

	  elt.cksum = cksum;

	  g_array_append_val (index, elt);

	  edsio_md5_init (& ctx);
	  memset (&cksum, 0, sizeof (cksum));
	  to_index = seg_len;
	}

      pos.off += consume;

      FLIP_FORWARD (pos);
    }

  if (! unmap_page (str, &pos))
    return NULL;

  {
    XdeltaRsync* rsync = g_new (XdeltaRsync, 1);

    rsync->seg_len = seg_len;
    rsync->file_len = handle_length (str);

    memcpy (rsync->file_md5, handle_checksum_md5 (str), 16);

    rsync->index = &g_array_index (index, XdeltaRsyncElt, 0);
    rsync->index_len = index->len;

    return rsync;
  }
}

static XdeltaRsync*
xdp_rsync_read_index (XdeltaStream*    cache_in)
{
  SerialSource* src = handle_source (cache_in);
  XdeltaRsync* rsync;

  if (! src)
    return NULL;

  if (! unserialize_rsyncindex (src, &rsync))
    return NULL;

  return rsync;
}

static gboolean
xdp_rsync_write_index (XdeltaRsync*     rsync,
		       XdeltaOutStream* cache_out)
{
  SerialSink* sink = handle_sink (cache_out, NULL, NULL, NULL, NULL);

  if (! sink)
    return FALSE;

  if (! serialize_rsyncindex_obj (sink, rsync))
    return FALSE;

  if (! handle_close (cache_out, 0))
    return FALSE;

  return TRUE;
}

XdeltaRsync*
xdp_rsync_index (XdeltaStream      *str,
		 guint              seg_len,
		 XdeltaStream      *cache_in,
		 XdeltaOutStream   *cache_out)
{
  XdeltaRsync* rsync;

  if (cache_in)
    {
      if (! (rsync = xdp_rsync_read_index (cache_in)))
	return NULL;

      if (seg_len != rsync->seg_len ||
	  (str && ! check_stream_integrity (str, rsync->file_md5, rsync->file_len)))
	{
	  xd_generate_void_event (EC_XdInvalidRsyncCache);
	  goto bail;
	}

      return rsync;
    }
  else
    {
      if (! (rsync = xdp_rsync_index_int (str, seg_len)))
	return NULL;

      if (cache_out)
	{
	  if (! xdp_rsync_write_index (rsync, cache_out))
	    goto bail;
	}

      return rsync;
    }

bail:

  xdp_rsync_index_free (rsync);

  return NULL;
}

void
xdp_rsync_index_free (XdeltaRsync *rsync)
{
  /* ??? */
}

static
gboolean xdp_rsync_hash (XdeltaRsync* rsync)
{
  guint i, index, prime = 0;
  gboolean already_hashed = rsync->table != NULL;
  SerialRsyncIndexElt** table = NULL;

  if (! already_hashed)
    {
      prime = rsync->table_size = g_spaced_primes_closest (rsync->index_len);
      table = rsync->table = g_new0 (SerialRsyncIndexElt*, prime);
    }

  for (i = 0; i < rsync->index_len; i += 1)
    {
      SerialRsyncIndexElt* elt = rsync->index + i;

      elt->match_offset = -1;

      if (! already_hashed)
	{
	  index = c_hash (& elt->cksum) % prime;

	  elt->next = table[index];
	  table[index] = elt;
	}
    }

  return TRUE;
}

static void
incr_by (XdeltaPos* pos, gint incr)
{
  do
    {
      gint rem = MIN (incr, pos->mem_rem - pos->off);

      pos->off += incr;
      incr -= rem;
      FLIP_FORWARD (*pos);
    }
  while (incr > 0 && pos->mem_rem != pos->page_size);
}

GArray*
xdp_rsync_request (XdeltaStream      *file,
		   XdeltaRsync       *rsync)
{
  XdeltaPos opos, npos;
  XdeltaChecksum cksum;
  guint max_buffer_index = handle_length (file);
  GArray *request = g_array_new (FALSE, FALSE, sizeof (guint));
  const guint8* n_pointer, *o_pointer;
  guint thistime;
  guint prime, index;
  SerialRsyncIndexElt **table;
  guint i;
  guint matched = 0;
  guint16 old_c, new_c;

  if (max_buffer_index < rsync->seg_len)
    return request;

  max_buffer_index -= rsync->seg_len;

  if (! xdp_rsync_hash (rsync))
    return NULL;

  g_assert (rsync->seg_len < handle_pagesize (file));

  init_pos (file, &opos);
  init_pos (file, &npos);
  memset (&cksum, 0, sizeof (cksum));

  prime = rsync->table_size;
  table = rsync->table;

  if (!map_page (file, &opos))
    return NULL;

  init_long_checksum (opos.mem, rsync->seg_len, &cksum);

  npos.off += rsync->seg_len;

  for (; XPOS (opos) < max_buffer_index; )
    {
      if (!map_page (file, &opos))
	return FALSE;

      if (!map_page (file, &npos))
	return FALSE;

      if (matched == rsync->index_len)
	break;

      thistime = MIN (opos.mem_rem - opos.off,
		      npos.mem_rem - npos.off);

      o_pointer = opos.mem + opos.off;
      n_pointer = npos.mem + npos.off;

      for (; ; o_pointer += 1, n_pointer += 1)
	{
	  index = c_hash (&cksum) % prime;

	  if (table[index])
	    {
	      gboolean md5_computed = FALSE;
	      gboolean found = FALSE;
	      guint8 md5[16];
	      SerialRsyncIndexElt* elt;

	      for (elt = table[index]; elt; elt = elt->next)
		{
		  if (elt->match_offset >= 0)
		    continue;

		  if (elt->cksum.high != cksum.high ||
		      elt->cksum.low  != cksum.low)
		    continue;

		  if (! md5_computed)
		    {
		      EdsioMD5Ctx ctx;

		      edsio_md5_init (& ctx);

		      if (opos.page == npos.page)
			edsio_md5_update (& ctx, opos.mem + opos.off, rsync->seg_len);
		      else
			{
			  edsio_md5_update (& ctx, opos.mem + opos.off, opos.mem_rem - opos.off);
			  edsio_md5_update (& ctx, npos.mem, rsync->seg_len - (opos.mem_rem - opos.off));
			}

		      edsio_md5_final (md5, & ctx);

		      md5_computed = TRUE;
		    }

		  if (memcmp (md5, elt->md5, 16) == 0)
		    {
		      matched += 1;
		      found = TRUE;
		      elt->match_offset = XPOS (opos);
		    }
		}

	      if (found)
		{
		  incr_by (&opos, rsync->seg_len);
		  incr_by (&npos, rsync->seg_len);
		  goto reenter;
		}
	    }

	  if (thistime == 0)
	    goto nextpage;

	  thistime -= 1;
	  opos.off += 1;
	  npos.off += 1;

	  old_c = CHEW(*o_pointer);
	  new_c = CHEW(*n_pointer);

	  cksum.low -= old_c;
	  cksum.low += new_c;

	  cksum.high -= old_c * rsync->seg_len;
	  cksum.high += cksum.low;
	}

    nextpage:

      FLIP_FORWARD (opos);
      FLIP_FORWARD (npos);

    reenter:
      (void) 0;
    }

  for (i = 0; i < rsync->index_len; i += 1)
    {
      SerialRsyncIndexElt* elt = rsync->index + i;

      if (elt->match_offset < 0)
	{
#ifdef DEBUG_RSYNC_REQUEST
	  g_print ("request segment %d\n", i);
#endif
	  g_array_append_val (request, i);
	}
    }

  return request;
}

gboolean
xdp_apply_rsync_reply (XdeltaRsync       *rsync,
		       XdeltaStream      *from,
		       XdeltaStream      *reply,
		       XdeltaStream      *out)
{
  gint i;
  guint reply_offset = 0;

  for (i = 0; i < rsync->index_len; i += 1)
    {
      SerialRsyncIndexElt* elt = rsync->index + i;

      if (elt->match_offset >= 0)
	{
	  if (! handle_copy (from, out, elt->match_offset, rsync->seg_len))
	    return FALSE;
	}
      else
	{
	  if (! handle_copy (reply, out, reply_offset, rsync->seg_len))
	    return FALSE;

	  reply_offset += rsync->seg_len;
	}
    }

  if (! handle_copy (reply, out, reply_offset, rsync->file_len % rsync->seg_len))
    return FALSE;

  if (! handle_close (out, 0))
    return FALSE;

  if (! check_stream_integrity (out, rsync->file_md5, rsync->file_len))
    return FALSE;

  return TRUE;
}
