/* -*-Mode: C;-*-
 * $Id: simple.c 1.6 Wed, 31 Mar 1999 17:39:58 -0800 jmacd $
 *
 * Copyright (C) 1998, 1999, Josh MacDonald.
 * All Rights Reserved.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 */

#include "edsio.h"

/* Simple Source
 */

typedef struct _ByteArraySource ByteArraySource;

struct _ByteArraySource {
  SerialSource source;

  const guint8* in_data;
  guint         in_len;

  guint read_pos;
  guint flags;
};

static gboolean     byte_array_source_close        (SerialSource* source);
static gboolean     byte_array_source_read         (SerialSource* source, guint8 *ptr, guint32 len);
static void         byte_array_source_free         (SerialSource* source);

SerialSource*
edsio_simple_source (const guint8* data, guint len, guint flags)
{
  ByteArraySource* it = g_new0 (ByteArraySource, 1);
  SerialSource* src = (SerialSource*) it;

  serializeio_source_init (& it->source,
			   NULL,
			   byte_array_source_close,
			   byte_array_source_read,
			   byte_array_source_free,
			   NULL,
			   NULL);

  it->in_data = data;
  it->in_len = len;
  it->flags = flags;

  if (flags & SBF_Base64)
    src = serializeio_base64_source (src);

  if (flags & SBF_Checksum)
    src = serializeio_checksum_source (src);

  if (flags & SBF_Compress)
    src = serializeio_gzip_source (src);

  return src;
}

gboolean
byte_array_source_close (SerialSource* source)
{
  return TRUE;
}

gboolean
byte_array_source_read (SerialSource* source, guint8 *buf, guint32 len)
{
  ByteArraySource* ssource = (ByteArraySource*) source;

  if (len + ssource->read_pos > ssource->in_len)
    {
      edsio_generate_source_event (EC_EdsioSourceEof, source);
      return FALSE;
    }

  memcpy (buf, ssource->in_data + ssource->read_pos, len);

  ssource->read_pos += len;

  return TRUE;
}

void
byte_array_source_free (SerialSource* source)
{
  g_free (source);
}

/* BASE64 Sink
 */

typedef struct _ByteArraySink   ByteArraySink;

struct _ByteArraySink {
  SerialSink sink;

  GByteArray* out;

  gpointer    data;
  guint       flags;
  gboolean    free_result;
  void (* success) (gpointer data, GByteArray* result);
};

static gboolean     byte_array_sink_close          (SerialSink* sink);
static gboolean     byte_array_sink_write          (SerialSink* sink, const guint8 *ptr, guint32 len);
static void         byte_array_sink_free           (SerialSink* sink);

SerialSink*
edsio_simple_sink (gpointer data,
		   guint    flags,
		   gboolean free_result,
		   void (* success) (gpointer data, GByteArray* result),
		   GByteArray **result)
{
  ByteArraySink* it = g_new0 (ByteArraySink, 1);
  SerialSink* sink = (SerialSink*) it;

  serializeio_sink_init (&it->sink,
			 NULL,
			 byte_array_sink_close,
			 byte_array_sink_write,
			 byte_array_sink_free,
			 NULL);

  it->data = data;
  it->out = g_byte_array_new ();
  it->flags = flags;
  it->free_result = free_result;
  it->success = success;

  if (result)
    (*result) = it->out;

  if (flags & SBF_Base64)
    sink = serializeio_base64_sink (sink);

  if (flags & SBF_Checksum)
    sink = serializeio_checksum_sink (sink);

  if (flags & SBF_Compress)
    sink = serializeio_gzip_sink (sink);

  return sink;
}

gboolean
byte_array_sink_close (SerialSink* sink)
{
  ByteArraySink* ssink = (ByteArraySink*) sink;

  if (ssink->success)
    ssink->success (ssink->data, ssink->out);

  return TRUE;
}

gboolean
byte_array_sink_write (SerialSink* sink, const guint8 *ptr, guint32 len)
{
  ByteArraySink* ssink = (ByteArraySink*) sink;

  g_byte_array_append (ssink->out, ptr, len);

  return TRUE;
}

void
byte_array_sink_free (SerialSink* sink)
{
  ByteArraySink* ssink = (ByteArraySink*) sink;

  if (ssink->out && ssink->free_result)
    g_byte_array_free (ssink->out, TRUE);

  g_free (sink);
}
