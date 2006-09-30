/* -*-Mode: C;-*-
 * $Id: fh.c 1.3 Wed, 31 Mar 1999 17:39:58 -0800 jmacd $
 *
 * Copyright (C) 1998, 1999, Josh MacDonald.
 * All Rights Reserved.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 */

#include "edsio.h"

/* Handle source/sink impls
 */
typedef struct _HandleSerialSource HandleSerialSource;
typedef struct _HandleSerialSink HandleSerialSink;

struct _HandleSerialSource {
  SerialSource source;

  FileHandle* fh;
};

struct _HandleSerialSink {
  SerialSink sink;

  FileHandle* fh;
  gpointer data1;
  gpointer data2;
  gpointer data3;
  gboolean (* cont_onclose) (gpointer data1, gpointer data2, gpointer data3);
};

static SerialType handle_source_type           (SerialSource* source, gboolean set_allocation);
static gboolean   handle_source_close          (SerialSource* source);
static gboolean   handle_source_read           (SerialSource* source, guint8 *ptr, guint32 len);
static void       handle_source_free           (SerialSource* source);

static gboolean     handle_sink_type             (SerialSink* sink, SerialType type, guint len, gboolean set_allocation);
static gboolean     handle_sink_close            (SerialSink* sink);
static gboolean     handle_sink_write            (SerialSink* sink, const guint8 *ptr, guint32 len);
static void         handle_sink_free             (SerialSink* sink);

SerialSource*
handle_source (FileHandle* fh)
{
  HandleSerialSource* it = g_new0 (HandleSerialSource, 1);

  serializeio_source_init (&it->source,
			   handle_source_type,
			   handle_source_close,
			   handle_source_read,
			   handle_source_free,
			   NULL,
			   NULL);

  it->fh = fh;

  return &it->source;
}

SerialSink*
handle_sink (FileHandle* fh, gpointer data1, gpointer data2, gpointer data3, gboolean (* cont_onclose) (gpointer data1, gpointer data2, gpointer data3))
{
  HandleSerialSink* it = g_new0 (HandleSerialSink, 1);

  serializeio_sink_init (&it->sink,
			 handle_sink_type,
			 handle_sink_close,
			 handle_sink_write,
			 handle_sink_free,
			 NULL);

  it->fh = fh;
  it->data1 = data1;
  it->data2 = data2;
  it->data3 = data3;
  it->cont_onclose = cont_onclose;

  return &it->sink;
}

SerialType
handle_source_type (SerialSource* source, gboolean set_allocation)
{
  HandleSerialSource* ssource = (HandleSerialSource*) source;
  guint32 x;

  if (! ssource->fh->table->table_handle_getui (ssource->fh, &x))
    return ST_Error;

  if (set_allocation)
    {
      if (! ssource->fh->table->table_handle_getui (ssource->fh, &source->alloc_total))
	return ST_Error;
    }

  return x;
}

gboolean
handle_source_close (SerialSource* source)
{
  HandleSerialSource* ssource = (HandleSerialSource*) source;

  return ssource->fh->table->table_handle_close (ssource->fh, 0);
}

gboolean
handle_source_read (SerialSource* source, guint8 *ptr, guint32 len)
{
  HandleSerialSource* ssource = (HandleSerialSource*) source;

  return ssource->fh->table->table_handle_read (ssource->fh, ptr, len) == len;
}

void
handle_source_free (SerialSource* source)
{
  g_free (source);
}

gboolean
handle_sink_type (SerialSink* sink, SerialType type, guint len, gboolean set_allocation)
{
  HandleSerialSink* ssink = (HandleSerialSink*) sink;

  if (! ssink->fh->table->table_handle_putui (ssink->fh, type))
    return FALSE;

  if (set_allocation && ! ssink->fh->table->table_handle_putui (ssink->fh, len))
    return FALSE;

  return TRUE;
}

gboolean
handle_sink_close (SerialSink* sink)
{
  HandleSerialSink* ssink = (HandleSerialSink*) sink;

  if (ssink->fh->table->table_handle_close (ssink->fh, 0))
    {
      if (ssink->cont_onclose)
	return ssink->cont_onclose (ssink->data1, ssink->data2, ssink->data3);

      return TRUE;
    }

  return FALSE;
}

gboolean
handle_sink_write (SerialSink* sink, const guint8 *ptr, guint32 len)
{
  HandleSerialSink* ssink = (HandleSerialSink*) sink;

  return ssink->fh->table->table_handle_write (ssink->fh, ptr, len);
}

void
handle_sink_free (SerialSink* sink)
{
  g_free (sink);
}
