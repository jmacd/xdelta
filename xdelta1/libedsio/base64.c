/* -*-Mode: C;-*-
 * $Id: base64.c 1.14 Wed, 31 Mar 1999 17:39:58 -0800 jmacd $
 *
 * Copyright (C) 1998, 1999, Josh MacDonald.
 * All Rights Reserved.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 */

#include "edsio.h"

/* BASE64 Encoding
 */

static const unsigned char base64_table[64] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a',
  'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
  '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

static gint16 base64_inverse_table[128];

static void
init_inverse_table (void)
{
  static int i = 0;
  static int j = 0;

  for (; j < 128; j += 1)
    base64_inverse_table[j] = -1;

  for (; i < 64; i += 1)
    base64_inverse_table[base64_table[i]] = i;

  base64_inverse_table['='] = 0;
}

GByteArray*
edsio_base64_encode_region (const guint8* data, guint len)
{
  GByteArray* out = g_byte_array_new ();
  guint real_len;

  g_byte_array_set_size (out, (len+2)*4/3);

  real_len = out->len;

  if (! edsio_base64_encode_region_into (data, len, out->data, &real_len))
    {
      g_byte_array_free (out, TRUE);
      return NULL;
    }

  g_byte_array_set_size (out, real_len);

  return out;
}

gboolean
edsio_base64_encode_region_into (const guint8* data, guint len, guint8* out, guint *out_len)
{
  gint i;
  guint32 word = 0, count = 0;

  if ((*out_len) < (len + 2) * 4/3)
    {
      edsio_generate_void_event (EC_EdsioInvalidBase64Encoding);
      return FALSE;
    }

  *out_len = 0;

  for (i = 0; i < len; i += 1)
    {
      word |= data[i] << (8*(2-(count++)));

      if (count == 3)
	{
	  out[(*out_len)++] = base64_table[(word>>18) & 0x3f];
	  out[(*out_len)++] = base64_table[(word>>12) & 0x3f];
	  out[(*out_len)++] = base64_table[(word>> 6) & 0x3f];
	  out[(*out_len)++] = base64_table[(word    ) & 0x3f];

	  count = 0;
	  word = 0;
	}
    }

  if (count > 0)
    {
      out[(*out_len)++] = base64_table[(word>>18) & 0x3f];
      out[(*out_len)++] = base64_table[(word>>12) & 0x3f];
      out[(*out_len)++] = (count > 1) ? base64_table[(word>>6) & 0x3f] : '=';
      out[(*out_len)++] = '=';
    }

  return TRUE;
}

GByteArray*
edsio_base64_decode_region (const guint8* data, guint data_len)
{
  GByteArray* it = g_byte_array_new ();
  guint real_len;

  g_byte_array_set_size (it, data_len*3/4);

  real_len = it->len;

  if (! edsio_base64_decode_region_into (data, data_len, it->data, &real_len))
    {
      g_byte_array_free (it, TRUE);
      return NULL;
    }

  g_byte_array_set_size (it, real_len);

  return it;
}

gboolean
edsio_base64_decode_region_into (const guint8* data, guint len, guint8* out, guint *out_len)
{
  guint32 pos = 0;
  gboolean found_end = FALSE;
  gint found_end_at = 0;

  init_inverse_table ();

  if ((*out_len) < (len*3/4))
    {
      edsio_generate_void_event (EC_EdsioOutputBufferShort);
      return FALSE;
    }

  (*out_len) = 0;

  while (pos < len)
    {
      gint i, x;
      gint word = 0, junk = 0;

      if (len - pos < 4)
	{
	  edsio_generate_void_event (EC_EdsioInvalidBase64Encoding);
	  return FALSE;
	}

      for (i = 0; i < 4; i += 1)
	{
	  x = data[pos++];

	  if (x > 127 || base64_inverse_table[x] < 0)
	    {
	      edsio_generate_void_event (EC_EdsioInvalidBase64Encoding);
	      return FALSE;
	    }

	  if (x == '=')
	    {
	      if (! found_end)
		found_end_at = i;

	      found_end = TRUE;
	    }
	  else
	    {
	      if (found_end)
		{
		  edsio_generate_void_event (EC_EdsioInvalidBase64Encoding);
		  return FALSE;
		}

	      word |= base64_inverse_table[x] << (6*(3-i));
	    }
	}

      if (found_end)
	{
	  if (found_end_at < 2)
	    {
	      edsio_generate_void_event (EC_EdsioInvalidBase64Encoding);
	      return FALSE;
	    }

	  if (found_end_at == 2)
	    junk = 2;
	  else if (found_end_at == 3)
	    junk = 1;
	}
      else
	junk = 0;

      out[(*out_len)++] = (word >> 16) & 0xff;

      if (junk < 2)
	out[(*out_len)++] = (word >>  8) & 0xff;

      if (junk < 1)
	out[(*out_len)++] = (word >>  0) & 0xff;
    }

  return TRUE;
}

/* Base64 sink
 */
typedef struct _Base64Sink Base64Sink;

static gboolean base64_sink_close (SerialSink* sink);
static gboolean base64_sink_write (SerialSink* sink, const guint8 *ptr, guint32 len);
static void     base64_sink_free (SerialSink* sink);
static gboolean base64_sink_quantum (SerialSink* sink);

struct _Base64Sink
{
  SerialSink sink;

  SerialSink* out;

  guint32 word;
  guint32 count;
};

SerialSink*
serializeio_base64_sink   (SerialSink* out)
{
  Base64Sink* it = g_new0 (Base64Sink, 1);
  SerialSink* sink = (SerialSink*) it;

  serializeio_sink_init (sink,
			 NULL,
			 base64_sink_close,
			 base64_sink_write,
			 base64_sink_free,
			 base64_sink_quantum);

  it->out = out;

  return sink;
}

gboolean
base64_sink_write (SerialSink* fsink, const guint8 *ptr, guint32 len)
{
  guint32 i;

  Base64Sink* sink = (Base64Sink*) fsink;

  for (i = 0; i < len; )
    {
      if (sink->count == 3)
	{
	  guint8 out[4];

	  out[0] = base64_table[(sink->word>>18) & 0x3f];
	  out[1] = base64_table[(sink->word>>12) & 0x3f];
	  out[2] = base64_table[(sink->word>> 6) & 0x3f];
	  out[3] = base64_table[(sink->word    ) & 0x3f];

#if 0
	  g_print ("%02x %02x %02x -> %c%c%c%c (3)\n",
		   (sink->word>>16) & 0xff,
		   (sink->word>>8) & 0xff,
		   (sink->word>>0) & 0xff,
		   out[0],
		   out[1],
		   out[2],
		   out[3]);
#endif

	  if (! sink->out->sink_write (sink->out, out, 4))
	    return FALSE;

	  sink->count = 0;
	  sink->word = 0;
	}

      while (sink->count < 3 && i < len)
	sink->word |= ptr[i++] << (8*(2-(sink->count++)));
    }

  return TRUE;
}

gboolean
base64_sink_close (SerialSink* fsink)
{
  Base64Sink* sink = (Base64Sink*) fsink;

  if (sink->count == 3)
    {
      guint8 out[4];

      out[0] = base64_table[(sink->word>>18) & 0x3f];
      out[1] = base64_table[(sink->word>>12) & 0x3f];
      out[2] = base64_table[(sink->word>> 6) & 0x3f];
      out[3] = base64_table[(sink->word    ) & 0x3f];

#if 0
      g_print ("%02x %02x %02x -> %c%c%c%c (3)\n",
	       (sink->word>>16) & 0xff,
	       (sink->word>>8) & 0xff,
	       (sink->word>>0) & 0xff,
	       out[0],
	       out[1],
	       out[2],
	       out[3]);
#endif

      if (! sink->out->sink_write (sink->out, out, 4))
	return FALSE;

      sink->count = 0;
      sink->word = 0;
    }

  if (sink->count > 0)
    {
      guint8 out[4];

      out[0] = base64_table[(sink->word>>18) & 0x3f];
      out[1] = base64_table[(sink->word>>12) & 0x3f];
      out[2] = (sink->count > 1) ? base64_table[(sink->word>>6) & 0x3f] : '=';
      out[3] = '=';

#if 0
      g_print ("%02x %02x %02x -> %c%c%c%c (%d)\n",
	       (sink->word>>16) & 0xff,
	       (sink->word>>8) & 0xff,
	       (sink->word>>0) & 0xff,
	       out[0],
	       out[1],
	       out[2],
	       out[3],
	       sink->count);
#endif

      if (! sink->out->sink_write (sink->out, out, 4))
	return FALSE;

      sink->count = 0;
      sink->word = 0;
    }

  return sink->out->sink_close (sink->out);
}


void
base64_sink_free (SerialSink* fsink)
{
  Base64Sink* sink = (Base64Sink*) fsink;

  sink->out->sink_free (sink->out);

  g_free (sink);
}

gboolean
base64_sink_quantum (SerialSink* fsink)
{
  Base64Sink* sink = (Base64Sink*) fsink;

  if (sink->out->sink_quantum)
    return sink->out->sink_quantum (sink->out);

  return TRUE;
}

/* Base64 source
 */

typedef struct _Base64Source Base64Source;

struct _Base64Source {
  SerialSource source;

  SerialSource *in;

  guint32 avail;
  guint32 count;
  gboolean found_end;
  gint found_end_at;
  guint8 buf[3];
};

static gboolean     base64_source_close        (SerialSource* source);
static gboolean     base64_source_read         (SerialSource* source, guint8 *ptr, guint32 len);
static void         base64_source_free         (SerialSource* source);

SerialSource*
serializeio_base64_source (SerialSource* in0)
{
  Base64Source* it = g_new0 (Base64Source, 1);
  SerialSource* source = (SerialSource*) it;

  serializeio_source_init (source,
			   NULL,
			   base64_source_close,
			   base64_source_read,
			   base64_source_free,
			   NULL,
			   NULL);

  it->in = in0;

  return source;
}

gboolean
base64_source_close (SerialSource* fsource)
{
  Base64Source* source = (Base64Source*) fsource;

  if (! source->in->source_close (source->in))
    return FALSE;

  return TRUE;
}

gboolean
base64_source_read (SerialSource* fsource, guint8 *ptr, guint32 len)
{
  guint32 pos;
  Base64Source* source = (Base64Source*) fsource;

  init_inverse_table ();

  for (pos = 0; pos < len; )
    {
      if (source->count == 0)
	{
	  guint8 buf[4];
	  guint32 i, word = 0, junk;

	  if (source->found_end)
	    {
	      edsio_generate_void_event (EC_EdsioInvalidBase64Encoding);
	      return FALSE;
	    }

	  if (! source->in->source_read (source->in, buf, 4))
	    return FALSE;

	  for (i = 0; i < 4; i += 1)
	    {
	      gint x = buf[i];

	      if (x > 127 || base64_inverse_table[x] < 0)
		{
		  edsio_generate_void_event (EC_EdsioInvalidBase64Encoding);
		  return FALSE;
		}

	      if (x == '=')
		{
		  if (! source->found_end)
		    source->found_end_at = i;

		  source->found_end = TRUE;
		}
	      else
		{
		  if (source->found_end)
		    {
		      edsio_generate_void_event (EC_EdsioInvalidBase64Encoding);
		      return FALSE;
		    }

		  word |= base64_inverse_table[x] << (6*(3-i));
		}
	    }

	  if (source->found_end)
	    {
	      if (source->found_end_at == 2)
		junk = 2;
	      else if (source->found_end_at == 3)
		junk = 1;
	      else
		{
		  edsio_generate_void_event (EC_EdsioInvalidBase64Encoding);
		  return FALSE;
		}
	    }
	  else
	    junk = 0;

	  source->avail = source->count = 3 - junk;

	  source->buf[0] = (word >> 16) & 0xff;
	  source->buf[1] = (word >>  8) & 0xff;
	  source->buf[2] = (word >>  0) & 0xff;

#if 0
	  g_print ("%c%c%c%c -> %02x %02x %02x (%d)\n",
		   buf[0],
		   buf[1],
		   buf[2],
		   buf[3],
		   (word>>16) & 0xff,
		   (word>>8) & 0xff,
		   (word>>0) & 0xff,
		   3-junk);
#endif
	}

      ptr[pos++] = source->buf[source->avail-(source->count--)];
    }

  return TRUE;
}

void
base64_source_free (SerialSource* fsource)
{
  Base64Source* source = (Base64Source*) fsource;

  source->in->source_free (source->in);

  g_free (source);
}
