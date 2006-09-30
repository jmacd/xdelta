/* -*-Mode: C;-*-
 * $Id: library.c 1.1 Tue, 06 Apr 1999 23:40:43 -0700 jmacd $
 *
 * Copyright (C) 1998, 1999, Josh MacDonald.
 * All Rights Reserved.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 */

#include "edsio.h"
#include <gmodule.h>

typedef struct _Library Library;

struct _Library {
  const char* name;
  const char* libname;
  gint        index;
  gboolean    loaded;
};

static Library known_libraries[] = {
  { "xd",      "xdelta",  3 },
  { "edsio",   "edsio",   6 },
};

static GHashTable* loaded_libraries;

static void
edsio_library_init ()
{
  if (! loaded_libraries)
    {
      gint i;
      gint n = sizeof (known_libraries) / sizeof (Library);;

      loaded_libraries = g_hash_table_new (g_int_hash, g_int_equal);

      for (i = 0; i < n; i += 1)
	{
	  Library* lib = known_libraries + i;

	  g_hash_table_insert (loaded_libraries, & lib->index, lib);
	}
    }
}

void
edsio_library_register (guint32 number, const char* name)
{
  Library* lib;

  edsio_library_init ();

  lib = g_hash_table_lookup (loaded_libraries, & number);

  if (lib)
    {
      lib->loaded = TRUE;
      return;
    }

  lib = g_new0 (Library, 1);

  lib->index = number;
  lib->name = name;
  lib->loaded = TRUE;

  g_hash_table_insert (loaded_libraries, & lib->index, lib);
}

gboolean
edsio_library_check (guint32 number)
{
  Library* lib;

  edsio_library_init ();

  lib = g_hash_table_lookup (loaded_libraries, & number);

  if (lib)
    {
      lib->loaded = TRUE;
      return TRUE;
    }

#if 0
  if (lib->libname && g_module_supported ())
    {
      GModule *module;
      GString *module_name = g_string_new (NULL);
      GString *symbol_name = g_string_new (NULL);
      gboolean (* init) (void);

      if (! (module = g_module_open (module_name->str, 0)))
	{
	  edsio_generate_stringstring_event (EC_EdsioGModuleError, module_name->str, g_module_error ());
	  return FALSE;
	}

      if (! g_module_symbol (module,
			     symbol_name->str,
			     (void**) & init))
	{
	  edsio_generate_stringstring_event (EC_EdsioGModuleError, g_module_name (module), g_module_error ());
	  return FALSE;
	}

      g_module_make_resident (module);

      g_module_close (module);

      lib->loaded = TRUE;

      return (* init) ();
    }
#endif

  edsio_generate_int_event (EC_EdsioUnregisteredLibrary, number);
  return FALSE;
}
