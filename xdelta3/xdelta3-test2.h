/* xdelta 3 - delta compression tools and library
 * Copyright (C) 2001, 2003, 2004, 2005, 2006, 2007.  Joshua P. MacDonald
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

/* This file was started because xdelta3-test.h is large enough as-is,
 * and has lots of old cruft.  This is the shiny new test I always wanted. */

static const int TESTS_PER_PARAMETER = 100;

struct random_file_spec_ {
  int size;
  char *tmp_copy;
  char *tmp_delta;
  mtrand mt;
};

struct random_parameters_ {
  int file_size;
  int window_size;
  int add_size;
  int del_size;
};

typedef struct random_file_spec_ random_file_spec;
typedef struct random_parameters_ random_parameters;

static const random_parameters test_parameters[] = {
  { 16384, 4096, 128, 0 },
  { 16384, 4096, 0, 128 },
  { 16384, 4096, 128, 128 },
  { 16384, 4096, 128, 128 },
};

static random_parameters current_parameters;
static int test2_malloc_count;

void 
set_test_parameters (const random_parameters *params)
{
  current_parameters = *params;
}

void 
set_random_parameters ()
{
  // TODO(jmacd)
  current_parameters = test_parameters[0];
}

void 
test2_free (void *ptr)
{
  test2_malloc_count--;
  free (ptr);
  XD3_ASSERT (test2_malloc_count >= 0);
}

void
random_file_spec_clear (random_file_spec *spec)
{
  if (spec->tmp_copy)
    {
      unlink (spec->tmp_copy);  /* TODO(jmacd): make portable */
      test2_free (spec->tmp_copy);
    }

  if (spec->tmp_delta)
    {
      unlink (spec->tmp_delta);
      test2_free (spec->tmp_delta);
    }
}

void 
random_file_spec_swap (random_file_spec *a, 
		       random_file_spec *b)
{
  random_file_spec t = *a;
  *a = *b;
  *b = t;
}

int
random_file_spec_generate (random_file_spec *spec) 
{
  return 0;
}

int
random_file_spec_write (random_file_spec *spec) 
{
  return 0;
}

int
random_file_spec_mutate (random_file_spec *from, random_file_spec *to)
{
  return 0;
}

int
random_file_spec_delta (random_file_spec *from, random_file_spec *to)
{
  return 0;
}

int
test_merge_chain (random_file_spec *specs, int number)
{
  /* "number" is from 1 (a single delta) between specs[0] and
   * specs[1], to N, an (N-1) chain from specs[0] to specs[N]. */
  return 0;
}

static int
test_merge_command ()
{
  /* Repeat random-input testing for a number of iterations.
   * Test 2, 3, and 4-file scenarios (i.e., 1, 2, and 3-delta merges). */
  int ret;
  int iter = 0, param = 0;
  random_file_spec spec[4];

  memset (spec, 0, sizeof (spec));

  /* Repeat this loop for TESTS_PER_PARAMETER * #parameters * 2.  The
   * first #parameters repeats are for the provided values, the second
   * set of repeats use random parameters. */
  for (; param < (2 * SIZEOF_ARRAY(test_parameters)); iter++)
    {
      if (iter % TESTS_PER_PARAMETER == 0)
	{
	  if (param < SIZEOF_ARRAY(test_parameters))
	    {
	      set_test_parameters (&test_parameters[param]);
	    } 
	  else 
	    {
	      set_random_parameters ();
	    }

	  param++;

	  if ((ret = random_file_spec_generate (&spec[0]))) { return ret; }
	  if ((ret = random_file_spec_write (&spec[0]))) { return ret; }

	  if ((ret = random_file_spec_mutate (&spec[0], &spec[1]))) { return ret; }
	  if ((ret = random_file_spec_write (&spec[1]))) { return ret; }
	  if ((ret = random_file_spec_delta (&spec[0], &spec[1]))) { return ret; }

	  if ((ret = random_file_spec_mutate (&spec[1], &spec[2]))) { return ret; }
	  if ((ret = random_file_spec_write (&spec[2]))) { return ret; }
	  if ((ret = random_file_spec_delta (&spec[1], &spec[2]))) { return ret; }
	}

      /* Each iteration creates a new mutation. */
      if ((ret = random_file_spec_mutate (&spec[2], &spec[3]))) { return ret; }
      if ((ret = random_file_spec_write (&spec[3]))) { return ret; }
      if ((ret = random_file_spec_delta (&spec[2], &spec[3]))) { return ret; }

      /* Test 1, 2, and 3 */
      if ((ret = test_merge_chain (spec, 1))) { return ret; }
      if ((ret = test_merge_chain (spec, 2))) { return ret; }
      if ((ret = test_merge_chain (spec, 3))) { return ret; }

      /* Clear 1st input, shift inputs */
      random_file_spec_clear (&spec[0]);
      random_file_spec_swap (&spec[0], &spec[1]);
      random_file_spec_swap (&spec[1], &spec[2]);
      random_file_spec_swap (&spec[2], &spec[3]);
    }

  return 0;
}
