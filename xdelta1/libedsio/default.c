/* -*-Mode: C;-*-
 * $Id: default.c 1.5 Wed, 31 Mar 1999 17:39:58 -0800 jmacd $
 *
 * Copyright (C) 1998, 1999, Josh MacDonald.
 * All Rights Reserved.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 */

#include "edsio.h"

/* Default Sink methods
 */

static gboolean
sink_type_default (SerialSink* sink, SerialType type, guint len, gboolean set_allocation)
{
  if (! sink->next_uint32 (sink, type))
    return FALSE;

  if (set_allocation && !sink->next_uint32 (sink, len))
    return FALSE;

  return TRUE;
}

static gboolean
sink_next_uint16 (SerialSink* sink, guint16 num)
{
  num = g_htons (num);

  return sink->sink_write (sink, (guint8*) &num, sizeof (num));
}

static gboolean
sink_next_uint32 (SerialSink* sink, guint32 num)
{
  num = g_htonl (num);

  return sink->sink_write (sink, (guint8*) &num, sizeof (num));
}

static gboolean
sink_next_uint (SerialSink* sink, guint32 num)
{
  /* This is mostly because I dislike endian, and less to save space
   * on small ints.  However, the uint32 and uint16 functions are used
   * when the number is expected to be large, in which case this
   * format can expand the number. */

  guint8 sink_buf[16];  /* this is enough room for a 12-byte int */
  guint  sink_count = 0;

  do
    {
      guint left = num & 0x7f;
      guint outnum;

      num >>= 7;

      outnum = left | (num ? 0x80 : 0);

      sink_buf[sink_count++] = outnum;
    }
  while (num);

  return sink->sink_write (sink, sink_buf, sink_count);
}

static gboolean
sink_next_uint8 (SerialSink* sink, guint8 val)
{
  return sink->sink_write (sink, &val, 1);
}

static gboolean
sink_next_bool (SerialSink* sink, gboolean val)
{
  guint8 sink_buf[1];
  sink_buf[0] = val;
  return sink->sink_write (sink, sink_buf, 1);
}

static gboolean
sink_next_string (SerialSink* sink, const char   *ptr)
{
  return sink->next_bytes (sink, ptr, strlen (ptr));
}

static gboolean
sink_next_bytes (SerialSink* sink, const guint8   *ptr, guint32 len)
{
  return sink->next_uint (sink, len) &&
         sink->sink_write (sink, ptr, len);
}

static gboolean
sink_next_bytes_known (SerialSink* sink, const guint8   *ptr, guint32 len)
{
  return sink->sink_write (sink, ptr, len);
}

void
serializeio_sink_init (SerialSink* it,
		       gboolean (* sink_type) (SerialSink* sink,
					       SerialType type,
					       guint mem_size,
					       gboolean set_allocation),
		       gboolean (* sink_close) (SerialSink* sink),
		       gboolean (* sink_write) (SerialSink* sink,
						const guint8 *ptr,
						guint32 len),
		       void     (* sink_free) (SerialSink* sink),
		       gboolean (* sink_quantum) (SerialSink* sink))
{
  it->next_bytes_known = sink_next_bytes_known;
  it->next_bytes = sink_next_bytes;
  it->next_uint = sink_next_uint;
  it->next_uint32 = sink_next_uint32;
  it->next_uint16 = sink_next_uint16;
  it->next_uint8 = sink_next_uint8;
  it->next_bool = sink_next_bool;
  it->next_string = sink_next_string;

  if (sink_type)
    it->sink_type = sink_type;
  else
    it->sink_type = sink_type_default;

  it->sink_close = sink_close;
  it->sink_write = sink_write;
  it->sink_free = sink_free;
  it->sink_quantum = sink_quantum;
}

/* Default Source methods
 */

static SerialType
source_type_default (SerialSource* source, gboolean set_allocation)
{
  guint32 x;

  if (! source->next_uint32 (source, & x))
    return ST_Error;

  if (set_allocation)
    {
      if (! source->next_uint32 (source, &source->alloc_total))
	return ST_Error;
    }

  return x;
}

static gboolean
source_next_uint32 (SerialSource* source, guint32 *ptr)
{
  guint32 x;

  if (! source->source_read (source, (guint8*) &x, sizeof (x)))
    return FALSE;

  (*ptr) = g_ntohl (x);

  return TRUE;
}

static gboolean
source_next_uint16 (SerialSource* source, guint16 *ptr)
{
  guint16 x;

  if (! source->source_read (source, (guint8*) &x, sizeof (x)))
    return FALSE;

  (*ptr) = g_ntohs (x);

  return TRUE;
}

static gboolean
source_next_uint (SerialSource* source, guint32 *ptr)
{
  /* This is mostly because I dislike endian, and less to save space
   * on small ints */
  guint8 c;
  guint8 arr[16];
  gint i = 0;
  gint donebit = 1;
  gint bits;

  while (source->next_uint8 (source, &c))
    {
      donebit = c & 0x80;
      bits = c & 0x7f;

      arr[i++] = bits;

      if (!donebit)
	break;
    }

  if (donebit)
    return FALSE;

  *ptr = 0;

  for (i -= 1; i >= 0; i -= 1)
    {
      *ptr <<= 7;
      *ptr |= arr[i];
    }

  return TRUE;
}

static gboolean
source_next_uint8 (SerialSource* source, guint8 *ptr)
{
  return source->source_read (source, ptr, 1);
}

static gboolean
source_next_bool (SerialSource* source, gboolean *ptr)
{
  guint8 sink_buf[1];

  if (! source->source_read (source, sink_buf, 1))
    return FALSE;

  if (sink_buf[0])
    *ptr = TRUE;
  else
    *ptr = FALSE;

  return TRUE;
}

static gboolean
source_next_string (SerialSource* source, const char **ptr)
{
  guint32 len;
  guint8* buf;

  if (! source->next_uint (source, &len))
    return FALSE;

  if (! (buf = serializeio_source_alloc (source, len+1)))
    return FALSE;

  buf[len] = 0;

  (*ptr) = buf;

  return source->source_read (source, buf, len);
}

static gboolean
source_next_bytes (SerialSource* source, const guint8 **ptr, guint32 *len_ptr)
{
  guint32 len;
  guint8* buf;

  if (! source->next_uint (source, &len))
    return FALSE;

  if (! (buf = serializeio_source_alloc (source, len)))
    return FALSE;

  (*len_ptr) = len;
  (*ptr) = buf;

  return source->source_read (source, buf, len);
}

static gboolean
source_next_bytes_known (SerialSource* source, guint8 *ptr, guint32 len)
{
  return source->source_read (source, ptr, len);
}

void*
serializeio_source_alloc (SerialSource* source, guint32 len)
{
  void* ret;

  if (! source->alloc_buf)
    {
      if (source->salloc_func)
	source->alloc_buf_orig = source->salloc_func (source, source->alloc_total + 8);
      else
	source->alloc_buf_orig = g_malloc0 (source->alloc_total + 8);

      source->alloc_buf = source->alloc_buf_orig;

	  { long x = source->alloc_buf; ALIGN_8 (x); source->alloc_buf = x; }

    }

  if (len+source->alloc_pos > source->alloc_total)
    {
      edsio_generate_source_event (EC_EdsioIncorrectAllocation, source);
      return NULL;
    }

  ret = ((guint8*)source->alloc_buf) + source->alloc_pos;

  source->alloc_pos += len;

  ALIGN_8 (source->alloc_pos);

  g_assert (((long)ret) % 8 == 0);
  g_assert (source->alloc_pos % 8 == 0);

  return ret;
}

gboolean
serializeio_source_object_received (SerialSource* source)
{
  source->alloc_pos = 0;
  source->alloc_total = 0;
  source->alloc_buf_orig = NULL;
  source->alloc_buf = NULL;

  return TRUE;
}

void
serializeio_source_reset_allocation (SerialSource* source)
{
  source->alloc_pos = 0;
  source->alloc_total = 0;
  source->alloc_buf = NULL;

  if (source->alloc_buf_orig)
    {
      if (source->sfree_func)
	source->sfree_func (source, source->alloc_buf_orig);
      else
	g_free (source->alloc_buf_orig);
    }
}

void
serializeio_source_init (SerialSource* it,
			 SerialType (* source_type) (SerialSource* source,
						     gboolean set_allocation),
			 gboolean   (* source_close) (SerialSource* source),
			 gboolean   (* source_read) (SerialSource* source,
						     guint8 *ptr,
						     guint32 len),
			 void       (* source_free) (SerialSource* source),
			 void*      (* salloc_func) (SerialSource* source,
						     guint32       len),
			 void       (* sfree_func) (SerialSource* source,
						    void*         ptr))
{
  it->next_bytes_known = source_next_bytes_known;
  it->next_bytes = source_next_bytes;
  it->next_uint = source_next_uint;
  it->next_uint32 = source_next_uint32;
  it->next_uint16 = source_next_uint16;
  it->next_uint8 = source_next_uint8;
  it->next_bool = source_next_bool;
  it->next_string = source_next_string;

  if (source_type != NULL)
    it->source_type = source_type;
  else
    it->source_type = source_type_default;
  it->source_close = source_close;
  it->source_read = source_read;
  it->source_free = source_free;
  it->salloc_func = salloc_func;
  it->sfree_func = sfree_func;
}
