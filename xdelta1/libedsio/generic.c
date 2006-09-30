/* -*-Mode: C;-*-
 * $Id: generic.c 1.3 Tue, 06 Apr 1999 23:40:10 -0700 jmacd $
 *
 * Copyright (C) 1998, 1999, Josh MacDonald.
 * All Rights Reserved.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 */

#include "edsio.h"

/* Type-based selectors for unknown types
 */

typedef struct {
  const char* name;
  gboolean (*unserialize_func) ();
  gboolean (*serialize_func) ();
  guint    (*count_func) ();
  void     (*print_func) ();
  guint32    val;
} SerEntry;

static GArray   *ser_array;
static gboolean  ser_array_sorted;

void serializeio_source_reset_allocation (SerialSource* source);

void
serializeio_initialize_type (const char* name,
			     guint32    val,
			     gboolean (*unserialize_func) (),
			     gboolean (*serialize_func) (),
			     guint    (*count_func) (),
			     void      (*print_func) ())
{
  SerEntry it;

  it.name = name;
  it.val = val;
  it.unserialize_func = unserialize_func;
  it.serialize_func = serialize_func;
  it.count_func = count_func;
  it.print_func = print_func;

  if (ser_array == NULL)
    ser_array = g_array_new (FALSE, TRUE, sizeof (SerEntry));

  g_array_append_val (ser_array, it);

  ser_array_sorted = FALSE;
}

static int
ser_entry_compare (const void* va, const void* vb)
{
  SerEntry* a = (SerEntry*) va;
  SerEntry* b = (SerEntry*) vb;

  return a->val - b->val;
}

static SerEntry*
serializeio_find_entry (SerialType type)
{
  if (! edsio_library_check (type & EDSIO_LIBRARY_OFFSET_MASK))
    return NULL;

  if (ser_array)
    {
      gint high_index = ser_array->len;
      gint low_index = 0;
      gint index;
      gint this_val;

      if (! ser_array_sorted)
	{
	  ser_array_sorted = TRUE;
	  qsort (ser_array->data, ser_array->len, sizeof (SerEntry), ser_entry_compare);
	}

    again:

      index = (low_index + high_index) / 2;

      this_val = g_array_index (ser_array, SerEntry, index).val;

      if (this_val < type)
	{
	  low_index = index + 1;
	  goto again;
	}
      else if (this_val > type)
	{
	  high_index = index - 1;
	  goto again;
	}
      else
	{
	  return & g_array_index (ser_array, SerEntry, index);
	}
    }

  edsio_generate_intint_event (EC_EdsioUnregisteredType,
			       type & EDSIO_LIBRARY_OFFSET_MASK,
			       type >> EDSIO_LIBRARY_OFFSET_BITS);
  return NULL;
}

gboolean
serializeio_unserialize_generic_internal (SerialSource *source,
					  SerialType   *object_type,
					  void        **object,
					  gboolean      set_allocation)
{
  SerialType type = (* source->source_type) (source, set_allocation);
  SerEntry* ent;
  gboolean res = FALSE;

  if (type < 0)
    return FALSE;

  ent = serializeio_find_entry (type);

  (*object_type) = type;

  if (ent)
    {
      res = ent->unserialize_func (source, object);

      if (set_allocation && res)
	{
	  if (! serializeio_source_object_received (source))
	    return FALSE;
	}
    }

  if (set_allocation)
    serializeio_source_reset_allocation (source);

  return res;
}

gboolean
serializeio_unserialize_generic (SerialSource *source,
				 SerialType   *object_type,
				 void        **object)
{
  return serializeio_unserialize_generic_internal (source, object_type, object, TRUE);
}

gboolean
serializeio_serialize_generic (SerialSink    *sink,
			       SerialType     object_type,
			       void          *object)
{
  return serializeio_serialize_generic_internal (sink, object_type, object, TRUE);
}

gboolean
serializeio_serialize_generic_internal (SerialSink    *sink,
					SerialType     object_type,
					void          *object,
					gboolean       set_allocation)
{
  SerEntry* ent;
  gboolean res = FALSE;

  if (! (* sink->sink_type) (sink, object_type, set_allocation ? serializeio_generic_count (object_type, object) : 0, set_allocation))
    return FALSE;

  ent = serializeio_find_entry (object_type);

  if (ent)
    res = ent->serialize_func (sink, object);

  return res;
}

const char*
serializeio_generic_type_to_string (SerialType type)
{
  SerEntry* ent;
  const char* res = "*Unknown*";

  ent = serializeio_find_entry (type);

  if (ent)
    res = ent->name;

  return res;
}

guint
serializeio_generic_count (SerialType     object_type,
			   void          *object)
{
  SerEntry* ent;
  gboolean res = FALSE;

  ent = serializeio_find_entry (object_type);

  if (ent)
    res = ent->count_func (object);

  return res;
}

void
serializeio_generic_print (SerialType type, void* object, guint indent_spaces)
{
  SerEntry* ent;

  ent = serializeio_find_entry (type);

  if (ent)
    ent->print_func (object, indent_spaces);
  else
    {
      int i = 0;

      for (; i < indent_spaces; i += 1)
	g_print (" ");

      g_print ("*Type Not Registered*\n");
    }
}

gboolean
serializeio_unserialize_generic_acceptable (SerialSource *source,
					    guint32       accept,
					    SerialType   *object_type,
					    void        **object)
{
  gboolean s;

  s = serializeio_unserialize_generic (source, object_type, object);

  if (s)
    {
      if (accept != -1)
	{
	  if ((*object_type & EDSIO_LIBRARY_OFFSET_MASK) != (accept & EDSIO_LIBRARY_OFFSET_MASK))
	    {
	      edsio_generate_intint_event (EC_EdsioUnexpectedLibraryType,
					   accept & EDSIO_LIBRARY_OFFSET_MASK,
					   *object_type & EDSIO_LIBRARY_OFFSET_MASK);

	      return FALSE;
	    }

	  if (! ((*object_type & ~EDSIO_LIBRARY_OFFSET_MASK) |
		 (accept       & ~EDSIO_LIBRARY_OFFSET_MASK)))
	    {
	      edsio_generate_void_event (EC_EdsioUnexpectedType);

	      return FALSE;
	    }
	}
    }

  return s;
}
