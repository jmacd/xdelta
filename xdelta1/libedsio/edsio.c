/* -*-Mode: C;-*-
 * $Id: edsio.c 1.24.1.1 Mon, 11 Jun 2001 01:56:01 -0700 jmacd $
 *
 * Copyright (C) 1998, 1999, Josh MacDonald.
 * All Rights Reserved.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 */

#include "edsio.h"
#include <stdio.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "maketime.h"

/*#define DEBUG_PROPS*/

/* Event delivery
 */

static GHashTable* all_event_defs = NULL;

static GPtrArray* all_event_watchers = NULL;

typedef struct _ErrorDeliveryWatcher ErrorDeliveryWatcher;
typedef struct _DelayedEvent DelayedEvent;

struct _ErrorDeliveryWatcher {
  ErrorDeliveryFunc deliver;
};

struct _DelayedEvent {
  GenericEvent     ev;
  GenericEventDef *def;
  const char      *msg;
};

void
eventdelivery_initialize_event_def (gint        code,
				    gint        level,
				    gint        flags,
				    const char* name,
				    const char* oneline,
				    const char * (* field_to_string) (GenericEvent* ev, gint field))
{
  GenericEventDef* def = g_new0 (GenericEventDef, 1);

  if (! all_event_defs)
    all_event_defs = g_hash_table_new (g_int_hash, g_int_equal);

  def->code = code;
  def->level = level;
  def->flags = flags;
  def->name = name;
  def->oneline = oneline;
  def->field_to_string = field_to_string;

  g_hash_table_insert (all_event_defs, & def->code, def);
}

void
eventdelivery_event_watch_all (ErrorDeliveryFunc func)
{
  ErrorDeliveryWatcher* w = g_new0 (ErrorDeliveryWatcher, 1);

  w->deliver = func;

  if (! all_event_watchers)
    all_event_watchers = g_ptr_array_new ();

  g_ptr_array_add (all_event_watchers, w);
}

GenericEventDef*
eventdelivery_event_lookup (gint code)
{
  return g_hash_table_lookup (all_event_defs, & code);
}

void
eventdelivery_event_deliver (GenericEvent* e)
{
  static gint in_call = FALSE;
  static GQueue* queued = NULL;
  static GPtrArray* free_strings = NULL;

  if (! queued)
    {
      queued = g_queue_new ();
      free_strings = g_ptr_array_new ();
    }

  in_call += 1;

  g_assert (e);

  edsio_edsio_init ();

  if (all_event_defs)
    {
      GenericEventDef* def = g_hash_table_lookup (all_event_defs, & e->code);

      if (def)
	{
	  GString* out;
	  const char* oneline = def->oneline;
	  char c;

	  out = g_string_new (NULL);

	  while ((c = *oneline++))
	    {
	      switch (c)
		{
		case '$':
		  {
		    const char* field;
		    char *end;
		    int f;

		    if ((*oneline++) != '{')
		      goto badevent;

		    f = strtol (oneline, &end, 10);

		    if (f < 0 || !end || end[0] != '}')
		      goto badevent;

		    oneline = end+1;

		    g_assert (def->field_to_string);

		    field = def->field_to_string (e, f);

		    if (field)
		      {
			g_string_append (out, field);

			g_free ((void*) field);
		      }
		    else
		      goto badevent;
		  }
		  break;
		default:
		  g_string_append_c (out, c);
		}
	    }

  	  if (! all_event_watchers)
	    {
	      fprintf (stderr, "%s:%d: %s\n", e->srcfile, e->srcline, out->str);

	      g_string_free (out, TRUE);
	    }
	  else if (in_call == 1)
	    {
	      gint i;

	      for (i = 0; i < all_event_watchers->len; i += 1)
		{
		  ErrorDeliveryWatcher* w = all_event_watchers->pdata[i];

		  if (! w->deliver (e, def, out->str))
		    {
		      g_warning ("%s:%d: An error delivery routine failed: %s\n", e->srcfile, e->srcline, out->str);
		      in_call = 0;
		      return;
		    }
		}

	      while (g_queue_get_size (queued) > 0)
		{
		  DelayedEvent* de = g_queue_pop (queued);

		  for (i = 0; i < all_event_watchers->len; i += 1)
		    {
		      ErrorDeliveryWatcher* w = all_event_watchers->pdata[i];

		      if (! w->deliver (& de->ev, de->def, de->msg))
			{
			  g_warning ("%s:%d: An error delivery routine failed: %s\n", e->srcfile, e->srcline, out->str);
			  in_call = 0;
			  return;
			}
		    }
		}

	      for (i = 0; i < free_strings->len; i += 1)
		g_string_free (free_strings->pdata[i], TRUE);

	      g_ptr_array_set_size (free_strings, 0);

	      g_string_free (out, TRUE);
	    }
	  else
	    {
	      DelayedEvent* de = g_new (DelayedEvent, 1);

	      de->ev = *e;
	      de->def = def;
	      de->msg = out->str;

	      g_queue_push (queued, de);

	      g_ptr_array_add (free_strings, out);
	    }

	      in_call -= 1;

	  return;
	}
    }

  g_warning ("%s:%d: Unrecognized event delivered (code=%d)\n", e->srcfile, e->srcline, e->code);

  in_call -= 1;

  return;

 badevent:

  g_warning ("%s:%d: An malformed error could not print here (code=%d)\n", e->srcfile, e->srcline, e->code);

  in_call -= 1;

  return;
}

const char*
eventdelivery_int_to_string (int x)
{
  return g_strdup_printf ("%d", x);
}

const char*
eventdelivery_string_to_string  (const char* x)
{
  return g_strdup (x);
}

const char*
eventdelivery_source_to_string  (SerialSource* x)
{
  return g_strdup ("@@@SerialSource");
}

const char*
eventdelivery_sink_to_string  (SerialSink* x)
{
  return g_strdup ("@@@SerialSink");
}

const char*
eventdelivery_handle_to_string (FileHandle* x)
{
  g_return_val_if_fail (x, g_strdup ("*error*"));

  return x->table->table_handle_name (x);
}

/* Misc crap.
 */

gboolean
edsio_time_of_day (SerialGenericTime* setme)
{
#if HAVE_GETTIMEOFDAY

  struct timeval tv;

  if (gettimeofday (& tv, NULL))
    {
      edsio_generate_errno_event (EC_EdsioGetTimeOfDayFailure);
      goto bail;
    }

  if (setme)
    {
      setme->nanos = tv.tv_usec * 1000;
      setme->seconds = tv.tv_sec;
    }

#else

  struct timeval tv;
  time_t t = time (NULL);

  if (t < 0)
    {
      edsio_generate_errno_event (EC_EdsioTimeFailure);
      goto bail;
    }

  if (setme)
    {
      setme->nanos = 0;
      setme->seconds = tv.tv_sec;
    }

#endif

  return TRUE;

 bail:

  setme->nanos = 0;
  setme->seconds = 10;

  return FALSE;
}

gchar*
edsio_time_to_iso8601 (SerialGenericTime *tp)
{
  return edsio_time_t_to_iso8601 (tp->seconds);
}

gchar*
edsio_time_t_to_iso8601 (GTime t0)
{
  static char timebuf[64];
  time_t t = t0;

  struct tm lt = *localtime(&t);
  int utc_offset = difftm(&lt, gmtime(&t));
  char sign = utc_offset < 0 ? '-' : '+';
  int minutes = abs (utc_offset) / 60;
  int hours = minutes / 60;

  sprintf(timebuf,
	  "%d-%02d-%02d %02d:%02d:%02d%c%02d%02d",
	  lt.tm_year + 1900,
	  lt.tm_mon + 1,
	  lt.tm_mday,
	  lt.tm_hour,
	  lt.tm_min,
	  lt.tm_sec,
	  sign,
	  hours,
	  minutes % 60);

  return timebuf;
}

static gboolean
strtosl_checked (const char* str, long* l, const char* errmsg)
{
  char* end;

  (*l) = strtol (str, &end, 10);

  if (!end || end[0])
    {
      if (errmsg)
	edsio_generate_stringstring_event (EC_EdsioInvalidIntegerString, errmsg, str);

      (*l) = 0;
      return FALSE;
    }

  return TRUE;
}

gboolean
strtosi_checked (const char* str, gint32* i,  const char* errmsg)
{
  long l;

  if (! strtosl_checked (str, &l, errmsg))
    {
      (*i) = 0;
      return FALSE;
    }

  if (l > G_MAXINT || l < G_MININT)
    {
      if (errmsg)
	edsio_generate_stringstring_event (EC_EdsioIntegerOutOfRange, errmsg, str);

      (*i) = 0;
      return FALSE;
    }

  (*i) = l;

  return TRUE;
}

gboolean
strtoss_checked (const char* str, gint16* i,  const char* errmsg)
{
  long l;

  if (! strtosl_checked (str, &l, errmsg))
    {
      (*i) = 0;
      return FALSE;
    }

  if (l > G_MAXSHORT || l < G_MINSHORT)
    {
      if (errmsg)
	edsio_generate_stringstring_event (EC_EdsioIntegerOutOfRange, errmsg, str);

      (*i) = 0;
      return FALSE;
    }

  (*i) = l;

  return TRUE;
}

gboolean
strtoui_checked (const char* str, guint32* i, const char* errmsg)
{
  long l;

  if (! strtosl_checked (str, &l, errmsg))
    {
      (*i) = 0;
      return FALSE;
    }

  if (l < 0)
    {
      if (errmsg)
	edsio_generate_stringstring_event (EC_EdsioInvalidIntegerSign, errmsg, str);

      (*i) = 0;
      return FALSE;
    }

  (*i) = l;

  if (l != (*i))
    {
      if (errmsg)
	edsio_generate_stringstring_event (EC_EdsioIntegerOutOfRange, errmsg, str);

      (*i) = 0;
      return FALSE;
    }

  return TRUE;
}

gboolean
strtous_checked (const char* str, guint16* i, const char* errmsg)
{
  long l;

  if (! strtosl_checked (str, &l, errmsg))
    {
      (*i) = 0;
      return FALSE;
    }

  if (l < 0)
    {
      if (errmsg)
	edsio_generate_stringstring_event (EC_EdsioInvalidIntegerSign, errmsg, str);

      (*i) = 0;
      return FALSE;
    }

  (*i) = l;

  if (l != (*i))
    {
      if (errmsg)
	edsio_generate_stringstring_event (EC_EdsioIntegerOutOfRange, errmsg, str);

      (*i) = 0;
      return FALSE;
    }

  return TRUE;
}

gint
edsio_md5_equal (gconstpointer   v,
		 gconstpointer   v2)
{
  return memcmp (v, v2, 16) == 0;
}

guint
edsio_md5_hash  (gconstpointer   v)
{
  guint8* md5 = (guint8*) v;
  guint x = 0;
  gint i, j;

  for (i = 0, j = 0; i < 16; i += 1, j += 1, j %= sizeof (guint))
    x ^= md5[i] << (8*j);

  return x;
}

void
serializeio_print_bytes (const guint8* bytes, guint len0)
{
  char buf[100];
  int i;
  guint len;

  len = MIN (len0, 32);

  for (i = 0; i < len; i += 1)
    sprintf (buf + 2*i, "%02x", bytes[i]);

  if (len0 > len)
    strcat (buf, "...");

  g_print ("%s\n", buf);
}

void
edsio_md5_to_string (const guint8* md5, char buf[33])
{
  gint i;

  for (i = 0; i < 16; i += 1)
    sprintf (buf + 2*i, "%02x", md5[i]);
}

static gboolean
from_hex (char c, int* x, const char* ctx)
{
  char buf[2];

  if (c >= '0' && c <= '9')
    {
      (*x) = c - '0';
      return TRUE;
    }
  else if (c >= 'A' && c <= 'F')
    {
      (*x) = c - 'A' + 10;
      return TRUE;
    }
  else if (c >= 'a' && c <= 'f')
    {
      (*x) = c - 'a' + 10;
      return TRUE;
    }

  buf[0] = c;
  buf[1] = 0;

  edsio_generate_stringstring_event (EC_EdsioInvalidHexDigit, buf, ctx);
  return FALSE;
}

gboolean
edsio_md5_from_string (guint8* md5, const char buf[33])
{
  gint i;
  gint l = strlen (buf);

  if (l < 32)
    {
      edsio_generate_string_event (EC_EdsioMD5StringShort, buf);
      return FALSE;
    }
  else if (l > 32)
    {
      edsio_generate_string_event (EC_EdsioMD5StringLong, buf);
      return FALSE;
    }

  for (i = 0; i < 16; i += 1)
    {
      char c1 = buf[(2*i)];
      char c2 = buf[(2*i)+1];
      int x1, x2;

      if (! from_hex (c1, &x1, buf))
	return FALSE;

      if (! from_hex (c2, &x2, buf))
	return FALSE;

      md5[i] = (x1 << 4) | x2;
    }

  return TRUE;
}

/* Strings
 */

const char* edsio_intern_string (const char* str)
{
  static GStringChunk* chunk = NULL;

  if (! chunk)
    chunk = g_string_chunk_new (256);

  return g_string_chunk_insert_const (chunk, str);
}

/* Properties
 */

typedef struct _EdsioHostType     EdsioHostType;
typedef struct _EdsioPropertyType EdsioPropertyType;

struct _EdsioPropertyType {
  const char *type_name;
  PropFreeFunc freer;
  PropGSFunc getter;
  PropGSFunc setter;
  PropSerialize serialize;
  PropUnserialize unserialize;
};

struct _EdsioHostType {
  const char *host_name;
  PropertyTableFunc ptable;
  PersistSourceFunc source;
  PersistSinkFunc   sink;
  PersistIssetFunc  isset;
  PersistUnsetFunc  unset;
};

struct _EdsioProperty {
  guint32     prop_code;
  const char *prop_name;
  guint32     prop_flags;
  EdsioPropertyType *type;
  EdsioHostType *host;
};

union _EdsioPropertyEntry {
  guint32          as_uint32;
  SerialEdsioBytes as_bytes;
  gpointer         as_vptr;
  const char*      as_string;
};

struct _EdsioGenericProperty
{
  guint32 code;
};

static GHashTable* all_property_types = NULL;
static GHashTable* all_host_types = NULL;
static GHashTable* all_properties = NULL;
static GHashTable* all_property_codes = NULL;
static guint32     property_code_sequence = 0;

void
edsio_initialize_property_type (const char* t, PropFreeFunc freer, PropGSFunc getter, PropGSFunc setter, PropSerialize ser, PropUnserialize unser)
{
  EdsioPropertyType* type;

  t = edsio_intern_string (t);

  if (! all_property_types)
    all_property_types = g_hash_table_new (g_direct_hash, g_direct_equal);

  if ((type = g_hash_table_lookup (all_property_types, t)) != NULL)
    {
      if (getter != type->getter ||
	  setter != type->setter ||
	  ser != type->serialize ||
	  unser != type->unserialize)
	edsio_generate_string_event (EC_EdsioDuplicatePropertyTypeRegistered, t);
      return;
    }

  type = g_new0 (EdsioPropertyType, 1);

  type->type_name = t;
  type->freer = freer;
  type->getter = getter;
  type->setter = setter;
  type->serialize = ser;
  type->unserialize = unser;

  g_hash_table_insert (all_property_types, (gpointer) t, type);
}

void
edsio_initialize_host_type (const char*       ph,
			    PropertyTableFunc ptable,
			    PersistSourceFunc source,
			    PersistSinkFunc   sink,
			    PersistIssetFunc  isset,
			    PersistUnsetFunc  unset)
{
  EdsioHostType* host;

  ph = edsio_intern_string (ph);

  if (! all_host_types)
    all_host_types = g_hash_table_new (g_direct_hash, g_direct_equal);

  if (g_hash_table_lookup (all_host_types, ph))
    {
      edsio_generate_string_event (EC_EdsioDuplicateHostTypeRegistered, ph);
      return;
    }

  host = g_new0 (EdsioHostType, 1);

  host->host_name = ph;
  host->ptable = ptable;
  host->source = source;
  host->sink   = sink;
  host->isset  = isset;
  host->unset  = unset;

  g_hash_table_insert (all_host_types, (gpointer) ph, host);

}

gboolean
edsio_new_property (const char* name, const char* ph, const char* t, guint32 flags, EdsioGenericProperty *ret_prop)
{
  EdsioProperty* prop;
  EdsioPropertyType* type;
  EdsioHostType* host;

  name = edsio_intern_string (name);
  ph   = edsio_intern_string (ph);
  t    = edsio_intern_string (t);

  g_assert (all_property_types);

  if (! all_properties)
    {
      all_properties = g_hash_table_new (g_direct_hash, g_direct_equal);
      all_property_codes = g_hash_table_new (g_int_hash, g_int_equal);
    }

  if ((prop = g_hash_table_lookup (all_properties, name)) != NULL)
    {
      edsio_generate_string_event (EC_EdsioDuplicatePropertyNameRegistered, name);
      ret_prop->code = prop->prop_code;
      return TRUE;
    }

  if ((type = g_hash_table_lookup (all_property_types, t)) == NULL)
    {
      edsio_generate_string_event (EC_EdsioNoSuchPropertyType, t);
      return FALSE;
    }

  if ((host = g_hash_table_lookup (all_host_types, ph)) == NULL)
    {
      edsio_generate_string_event (EC_EdsioNoSuchHostType, ph);
      return FALSE;
    }

  if (flags & PF_Persistent && ! host->isset)
    {
      edsio_generate_stringstring_event (EC_EdsioPersistenceUnavailable, name, ph);
      return FALSE;
    }

  prop = g_new0 (EdsioProperty, 1);

  prop->prop_code  = ++property_code_sequence;
  prop->prop_name  = name;
  prop->prop_flags = flags;
  prop->type       = type;
  prop->host       = host;

  g_hash_table_insert (all_properties, (gpointer) name, prop);
  g_hash_table_insert (all_property_codes, & prop->prop_code, prop);

  ret_prop->code = prop->prop_code;

  return TRUE;
}

static EdsioProperty*
edsio_property_find (const char* ph, const char* t, guint32 code)
{
  EdsioProperty* prop;

  ph = edsio_intern_string (ph);
  t  = edsio_intern_string (t);

  if (code <= 0 || code > property_code_sequence)
    {
      edsio_generate_int_event (EC_EdsioNoSuchProperty, code);
      return NULL;
    }

  if (! (prop = g_hash_table_lookup (all_property_codes, & code)))
    {
      edsio_generate_int_event (EC_EdsioNoSuchProperty, code);
      return NULL;
    }

  if (prop->host->host_name != ph)
    {
      edsio_generate_stringstringstring_event (EC_EdsioWrongHostType, prop->prop_name, ph, prop->host->host_name);
      return NULL;
    }

  if (prop->type->type_name != t)
    {
      edsio_generate_stringstringstring_event (EC_EdsioWrongDataType, prop->prop_name, t, prop->type->type_name);
      return NULL;
    }

  return prop;
}

EdsioPropertyEntry*
edsio_property_get (gpointer obj, EdsioProperty* prop)
{
  EdsioPropertyEntry* ent;
  GHashTable* table = * prop->host->ptable (obj);
  gboolean persist = prop->prop_flags & PF_Persistent;

#ifdef DEBUG_PROPS
  g_print ("get %p.%s\n", obj, prop->prop_name);
#endif

  if (table && (ent = g_hash_table_lookup (table, & prop->prop_code)) != NULL)
    return ent;

  if (persist)
    {
      SerialSource* src;

      if (! (src = prop->host->source (obj, prop->prop_name)))
	return NULL;

      g_assert (prop->type->unserialize);

      if (! prop->type->unserialize (src, & ent))
	return NULL;

      g_assert (ent);

      if (! src->source_close (src))
	return NULL;

      src->source_free (src);

      if (! table)
	table = (* prop->host->ptable (obj)) = g_hash_table_new (g_int_hash, g_int_equal);

      g_hash_table_insert (table, & prop->prop_code, ent);

      return ent;
    }

  edsio_generate_string_event (EC_EdsioPropertyNotSet, prop->prop_name);
  return NULL;
}

gboolean
edsio_property_set (gpointer obj, EdsioProperty* prop, EdsioPropertyEntry* set)
{
  EdsioPropertyEntry* ent;
  gboolean persist = prop->prop_flags & PF_Persistent;
  GHashTable* table = * prop->host->ptable (obj);

#ifdef DEBUG_PROPS
  g_print ("set %p.%s\n", obj, prop->prop_name);
#endif

  if (! table)
    table = (* prop->host->ptable (obj)) = g_hash_table_new (g_int_hash, g_int_equal);

  ent = g_hash_table_lookup (table, & prop->prop_code);

  if (ent)
    {
      g_hash_table_remove (table, & prop->prop_code);
      prop->type->freer (ent);
    }

  g_hash_table_insert (table, & prop->prop_code, set);

  if (persist)
    {
      SerialSink* sink;

      if (! (sink = prop->host->sink (obj, prop->prop_name)))
	return FALSE;

      g_assert (prop->type->serialize);

      if (! prop->type->serialize (sink, set))
	return FALSE;

      if (! sink->sink_close (sink))
	return FALSE;

      sink->sink_free (sink);
    }

  return TRUE;
}

gboolean
edsio_property_isset (const char* ph, const char* t, guint32 code, gpointer obj)
{
  EdsioProperty* prop;
  GHashTable* table;
  gboolean persist;
  gboolean result = FALSE;

  if (! (prop = edsio_property_find (ph, t, code)))
    goto done;

  persist = prop->prop_flags & PF_Persistent;

  table = * prop->host->ptable (obj);

  if (persist)
    {
	  PersistIssetFunc issetfunc = prop->host->isset;
      if (issetfunc(obj, prop->prop_name))
	{
	  if (! edsio_property_get (obj, prop))
	    goto done;

	  table = * prop->host->ptable (obj);
	}
    }

  if (! table)
    goto done;

  result = (g_hash_table_lookup (table, & code) != NULL);

 done:

#ifdef DEBUG_PROPS
  g_print ("isset %p.%s = %s\n", obj, prop->prop_name, result ? "true" : "false");
#endif

  return result;
}

gboolean
edsio_property_unset (const char* ph, const char* t, guint32 code, gpointer obj)
{
  EdsioProperty* prop;
  gboolean persist;
  GHashTable* table;

  if (! (prop = edsio_property_find (ph, t, code)))
    return FALSE;

#ifdef DEBUG_PROPS
  g_print ("unset %p.%s\n", obj, prop->prop_name);
#endif

  persist = prop->prop_flags & PF_Persistent;
  table = * prop->host->ptable (obj);

  if (table)
    {
      EdsioPropertyEntry* ent;

      ent = g_hash_table_lookup (table, & code);

      g_hash_table_remove (table, & code);

      if (g_hash_table_size (table) == 0)
	{
	  g_hash_table_destroy (table);
	  table = (* prop->host->ptable (obj)) = NULL;
	}

      /*g_free (ent);*/
    }

  if (persist)
    {
      if (! prop->host->unset (obj, prop->prop_name))
	return FALSE;
    }

  return TRUE;
}

static gboolean
edsio_false ()
{
  return FALSE;
}

PropGSFunc
edsio_property_getter (const char* ph, const char* t, guint32 code, EdsioProperty** ep)
{
  if (! ((*ep) = edsio_property_find (ph, t, code)))
    return & edsio_false;

  return (* ep)->type->getter;
}

PropGSFunc
edsio_property_setter (const char* ph, const char* t, guint32 code, EdsioProperty** ep)
{
  if (! ((* ep) = edsio_property_find (ph, t, code)))
    return & edsio_false;

  return (* ep)->type->setter;
}

/* Primitive type serializers
 */

/* integer
 */
gboolean
edsio_property_uint_getter (gpointer obj, EdsioProperty* prop, guint32* get)
{
  EdsioPropertyEntry *ent;

  if (! (ent = edsio_property_get (obj, prop)))
    return FALSE;

  (*get) = ent->as_uint32;

  return TRUE;
}

gboolean
edsio_property_uint_setter (gpointer obj, EdsioProperty* prop, guint32  set)
{
  EdsioPropertyEntry *ent = g_new (EdsioPropertyEntry, 1);

  ent->as_uint32 = set;

  return edsio_property_set (obj, prop, ent);
}

void
edsio_property_uint_free (gpointer obj)
{
  g_free (obj);
}

gboolean
unserialize_uint (SerialSource *source, guint32** x)
{
  SerialEdsioUint *s;
  guint32 *n;

  if (! unserialize_edsiouint (source, & s))
    return FALSE;

  n = g_new (guint32, 1);

  (* x) = n;

  (* n) = s->val;

  g_free (s);

  return TRUE;
}

gboolean
serialize_uint_obj (SerialSink *sink, guint32* x)
{
  return serialize_edsiouint (sink, *x);
}

/* String
 */

void
edsio_property_string_free   (gpointer obj)
{
  g_free (obj);
}

gboolean
edsio_property_string_getter (gpointer obj, EdsioProperty* prop, const char** get)
{
  if (! ((*get) = (const char*) edsio_property_get (obj, prop)))
    return FALSE;

  return TRUE;
}

gboolean
edsio_property_string_setter (gpointer obj, EdsioProperty* prop, const char*  set)
{
  return edsio_property_set (obj, prop, (EdsioPropertyEntry*) set);
}

gboolean
unserialize_string   (SerialSource *source, const char** x)
{
  SerialEdsioString *s;

  if (! unserialize_edsiostring (source, & s))
    return FALSE;

  (*x) = g_strdup (s->val);

  g_free (s);

  return TRUE;
}

gboolean
serialize_string_obj (SerialSink *sink, const char* x)
{
  return serialize_edsiostring (sink, x);
}

/* Bytes
 */

gboolean
unserialize_bytes (SerialSource *source, SerialEdsioBytes** x)
{
  return unserialize_edsiobytes (source, x);
}

gboolean
serialize_bytes_obj (SerialSink *sink, SerialEdsioBytes *x)
{
  return serialize_edsiobytes_obj (sink, x);
}

gboolean
edsio_property_bytes_getter (gpointer obj, EdsioProperty* prop, guint8** get, guint32* get_len)
{
  EdsioPropertyEntry *ent;

  if (! (ent = edsio_property_get (obj, prop)))
    return FALSE;

  (* get) = (gpointer) ent->as_bytes.val;
  (* get_len) = ent->as_bytes.val_len;

  return TRUE;
}

gboolean
edsio_property_bytes_setter (gpointer obj, EdsioProperty* prop, guint8* set, guint32 set_len)
{
  EdsioPropertyEntry *ent = g_new (EdsioPropertyEntry, 1);

  ent->as_bytes.val = set;
  ent->as_bytes.val_len = set_len;

  return edsio_property_set (obj, prop, ent);
}

void
edsio_property_bytes_free (gpointer obj)
{
  g_free (obj);
}

/* Vptr
 */

gboolean
edsio_property_vptr_getter (gpointer obj, EdsioProperty* prop, void** get)
{
  if (! ((*get) = edsio_property_get (obj, prop)))
    return FALSE;

  return TRUE;
}

gboolean
edsio_property_vptr_setter (gpointer obj, EdsioProperty* prop, void* set)
{
  return edsio_property_set (obj, prop, (EdsioPropertyEntry*) set);
}

void
edsio_property_vptr_free (gpointer obj)
{
  /* nothing */
}

/* Testing
 */

#ifdef DEBUG_LIBEDSIO

GHashTable**
edsio_proptest_property_table (PropTest *pt)
{
  return & pt->_edsio_property_table;
}

SerialSource*
edsio_persist_proptest_source (PropTest *pt, const char* prop_name)
{
  GByteArray* array;

  if (! pt->ptable)
    {
      g_warning ("can't get persist property, no table\n");
      return NULL;
    }

  if (! (array = g_hash_table_lookup (pt->ptable, prop_name)))
    {
      g_warning ("can't lookup persist property\n");
      return NULL;
    }

  return edsio_simple_source (array->data, array->len, SBF_None);
}

static void
pt_success (gpointer data, GByteArray* result)
{
  PropTest* pt = data;

  GByteArray* old;

  if (! pt->ptable)
    pt->ptable = g_hash_table_new (g_str_hash, g_str_equal);

  old = g_hash_table_lookup (pt->ptable, (gpointer) pt->kludge);

  if (old)
    g_byte_array_free (old, TRUE);

  g_hash_table_insert (pt->ptable, (gpointer) pt->kludge, result);
}

SerialSink*
edsio_persist_proptest_sink   (PropTest *pt, const char* prop_name)
{
  pt->kludge = prop_name;

  return edsio_simple_sink (pt, SBF_None, FALSE, pt_success, NULL);
}

gboolean
edsio_persist_proptest_isset  (PropTest *pt, const char* prop_name)
{
  if (! pt->ptable)
    return FALSE;

  return g_hash_table_lookup (pt->ptable, prop_name) != NULL;
}

gboolean
edsio_persist_proptest_unset  (PropTest *pt, const char* prop_name)
{
  GByteArray* old;

  if (! pt->ptable)
    return FALSE;

  old = g_hash_table_lookup (pt->ptable, prop_name);

  if (old)
    {
      g_byte_array_free (old, TRUE);
      g_hash_table_remove (pt->ptable, prop_name);
      return TRUE;
    }

  return FALSE;
}

#endif

/* Misc source/sink stuff
 */

SerialSink*
serializeio_gzip_sink (SerialSink* sink)
{
  /* @@@ not implemented */
  return sink;
}

SerialSource*
serializeio_gzip_source (SerialSource* source)
{
  /* @@@ not implemented */
  return source;
}

/* Checksum sink
 */
typedef struct _ChecksumSink ChecksumSink;

static gboolean checksum_sink_close (SerialSink* sink);
static gboolean checksum_sink_write (SerialSink* sink, const guint8 *ptr, guint32 len);
static void     checksum_sink_free (SerialSink* sink);
static gboolean checksum_sink_quantum (SerialSink* sink);

struct _ChecksumSink
{
  SerialSink sink;

  SerialSink* out;

  EdsioMD5Ctx ctx;
  guint8      md5[16];
  gboolean    md5_done;
  gboolean    md5_written;
};

SerialSink*
serializeio_checksum_sink   (SerialSink* out)
{
  ChecksumSink* it = g_new0 (ChecksumSink, 1);
  SerialSink* sink = (SerialSink*) it;

  serializeio_sink_init (sink,
			 NULL,
			 checksum_sink_close,
			 checksum_sink_write,
			 checksum_sink_free,
			 checksum_sink_quantum);

  it->out = out;

  edsio_md5_init (& it->ctx);

  return sink;
}

gboolean
checksum_sink_write (SerialSink* fsink, const guint8 *ptr, guint32 len)
{
  ChecksumSink* sink = (ChecksumSink*) fsink;

  if (! sink->out->sink_write (sink->out, ptr, len))
    return FALSE;

  edsio_md5_update (& sink->ctx, ptr, len);

  return TRUE;
}

gboolean
checksum_sink_close (SerialSink* fsink)
{
  ChecksumSink* sink = (ChecksumSink*) fsink;

  if (! sink->md5_done)
    {
      edsio_md5_final (sink->md5, & sink->ctx);
      sink->md5_done = TRUE;
    }

  if (! sink->out->sink_write (sink->out, sink->md5, 16))
    return FALSE;

  if (! sink->out->sink_close (sink->out))
    return FALSE;

  return TRUE;
}

void
checksum_sink_free (SerialSink* fsink)
{
  ChecksumSink* sink = (ChecksumSink*) fsink;

  sink->out->sink_free (sink->out);

  g_free (sink);
}

gboolean
checksum_sink_quantum (SerialSink* fsink)
{
  ChecksumSink* sink = (ChecksumSink*) fsink;

  if (sink->out->sink_quantum)
    return sink->out->sink_quantum (sink->out);

  return TRUE;
}

/* Checksum source
 */

typedef struct _ChecksumSource ChecksumSource;

struct _ChecksumSource {
  SerialSource source;

  SerialSource *in;

  EdsioMD5Ctx ctx;
};

static gboolean     checksum_source_close        (SerialSource* source);
static gboolean     checksum_source_read         (SerialSource* source, guint8 *ptr, guint32 len);
static void         checksum_source_free         (SerialSource* source);

SerialSource*
serializeio_checksum_source (SerialSource* in0)
{
  ChecksumSource* it = g_new0 (ChecksumSource, 1);
  SerialSource* source = (SerialSource*) it;

  serializeio_source_init (source,
			   NULL,
			   checksum_source_close,
			   checksum_source_read,
			   checksum_source_free,
			   NULL,
			   NULL);

  it->in = in0;

  edsio_md5_init (& it->ctx);

  return source;
}

gboolean
checksum_source_close (SerialSource* fsource)
{
  ChecksumSource* source = (ChecksumSource*) fsource;
  guint8 buf1[16];
  guint8 buf2[16];

  if (! source->in->source_read (source->in, buf1, 16))
    return FALSE;

  edsio_md5_final (buf2, & source->ctx);

  if (memcmp (buf1, buf2, 16) != 0)
    {
      edsio_generate_void_event (EC_EdsioInvalidStreamChecksum);
      return FALSE;
    }

  if (! source->in->source_close (source->in))
    return FALSE;

  return TRUE;
}

gboolean
checksum_source_read (SerialSource* fsource, guint8 *ptr, guint32 len)
{
  ChecksumSource* source = (ChecksumSource*) fsource;

  if (! source->in->source_read (source->in, ptr, len))
    return FALSE;

  edsio_md5_update (& source->ctx, ptr, len);

  return TRUE;
}

void
checksum_source_free (SerialSource* fsource)
{
  ChecksumSource* source = (ChecksumSource*) fsource;

  source->in->source_free (source->in);

  g_free (source);
}

/* Missing glib stuff
 */

GQueue *
g_queue_new (void)
{
  GQueue *q = g_new (GQueue, 1);

  q->list = q->list_end = NULL;
  q->list_size = 0;

  return q;
}


void
g_queue_free (GQueue *q)
{
  if (q)
    {
      if (q->list)
        g_list_free (q->list);
      g_free (q);
    }
}


guint
g_queue_get_size (GQueue *q)
{
  return (q == NULL) ? 0 : q->list_size;
}


void
g_queue_push_front (GQueue *q, gpointer data)
{
  if (q)
    {
      q->list = g_list_prepend (q->list, data);

      if (q->list_end == NULL)
        q->list_end = q->list;

      q->list_size++;
    }
}


void
g_queue_push_back (GQueue *q, gpointer data)
{
  if (q)
    {
      q->list_end = g_list_append (q->list_end, data);

      if (! q->list)
        q->list = q->list_end;
      else
        q->list_end = q->list_end->next;

      q->list_size++;
    }
}


gpointer
g_queue_pop_front (GQueue *q)
{
  gpointer data = NULL;

  if ((q) && (q->list))
    {
      GList *node;

      node = q->list;
      data = node->data;

      if (! node->next)
        {
          q->list = q->list_end = NULL;
          q->list_size = 0;
        }
      else
        {
          q->list = node->next;
          q->list->prev = NULL;
          q->list_size--;
        }

      g_list_free_1 (node);
    }

  return data;
}


gpointer
g_queue_pop_back (GQueue *q)
{
  gpointer data = NULL;

  if ((q) && (q->list))
    {
      GList *node;

      node = q->list_end;
      data = node->data;

      if (! node->prev)
	{
          q->list = q->list_end = NULL;
          q->list_size = 0;
        }
      else
	{
          q->list_end = node->prev;
          q->list_end->next = NULL;
          q->list_size--;
        }

      g_list_free_1 (node);
    }

  return data;
}
