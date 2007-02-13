/* xdelta 3 - delta compression tools and library
 * Copyright (C) 2003, 2004, 2005, 2006, 2007.  Joshua P. MacDonald
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Python.h"

static PyObject *pyxd3_error;

/* spam: xdelta3.main([string,list,...]) */
PyObject *xdelta3_main_cmdline (PyObject *self, PyObject *args)
{
  int ret, i, nargs;
  char **argv = NULL;
  int argc = 0;
  PyObject *result = NULL;
  PyObject *o;

  if (! PyArg_ParseTuple (args, "O", &o)
      || ! PyList_Check (o))
    {
      goto cleanup;
    }

  argc  = PyList_Size (o);
  nargs = argc + 2;

  if (! (argv = malloc (sizeof(argv[0]) * nargs)))
    {
      PyErr_NoMemory ();
      goto cleanup;
    }
  memset (argv, 0, sizeof(argv[0]) * nargs);

  for (i = 1; i < nargs-1; i += 1)
    {
      char *ps;
      PyObject *s;
      if ((s = PyList_GetItem (o, i-1)) == NULL) { goto cleanup; }
      ps = PyString_AsString (s);
      /* TODO: ps is NULL if s is not a string, crashes the interpreter */
      argv[i] = ps;
    }

  ret = xd3_main_cmdline (argc+1, argv);

  if (ret == 0)
    {
      result = Py_BuildValue ("i", ret);
    }
  else
    {
      PyErr_SetString (pyxd3_error, "failed :(");
    }
 cleanup:
  if (argv)
    {
      free (argv);
    }
  return result;
}

static PyMethodDef xdelta3_methods[] = {
  { "main", xdelta3_main_cmdline, METH_VARARGS, "xdelta3 main()" },
  { NULL, NULL }
};

DL_EXPORT(void) initxdelta3main (void)
{
  PyObject *m, *d;
  m = Py_InitModule ("xdelta3main", xdelta3_methods);
  d = PyModule_GetDict (m);
  pyxd3_error = PyErr_NewException ("xdelta3main.error", NULL, NULL);
  PyDict_SetItemString (d, "error", pyxd3_error);
}
