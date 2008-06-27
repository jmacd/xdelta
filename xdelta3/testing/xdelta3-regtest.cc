/* xdelta 3 - delta compression tools and library
 * Copyright (C) 2001, 2003, 2004, 2005, 2006, 2007, 2008. Joshua P. MacDonald
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

// This file started because I wanted to write a test in C++ rather than C.

extern "C" {
#define NOT_MAIN 1
#define REGRESSION_TEST 1
#include "../xdelta3.c"
}

namespace {

const int TEST_SEED1 = 0x1970;

class RandomFileSpec {
 public:
  RandomFileSpec()
    : size_(0),
      seed_(0),
      filedata_(NULL),
      filename_(NULL),
      deltafile_(NULL) {
    main_file_init(&file_);
    mt_init(&mt_, TEST_SEED1);
  }
  int size_;
  int seed_;
  uint8_t *filedata_;
  char *filename_;
  char *deltafile_;
  main_file file_;
  mtrand mt_;
};

}  // namespace

int main(int argc, char **argv) {
  RandomFileSpec spec1;

  return 0;
}


#if 0
static const int TESTS_PER_PARAMETER = 100;

struct random_file_spec_ {
  int size;
  int seed;
  main_file file;
  uint8_t *tmp_data;
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
  { 16384, 4096, 16, 0 },
  { 16384, 4096, 0, 16 },
  { 16384, 4096, 16, 16 },
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

void*
test2_malloc (int size)
{
  test2_malloc_count++;
  return malloc(size);
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
      spec->tmp_copy = NULL;
    }

  if (spec->tmp_delta)
    {
      unlink (spec->tmp_delta);
      test2_free (spec->tmp_delta);
      spec->tmp_delta = NULL;
    }

  if (spec->tmp_data)
    {
      test2_free (spec->tmp_data);
      spec->tmp_data = NULL;
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
  int i;
  spec->seed = mt_random (&static_mtrand);
  mt_init (&spec->mt, spec->seed);
  main_file_init (&spec->file);

  test_setup();
  spec->tmp_copy = test2_malloc(strlen(TEST_TARGET_FILE) + 1);
  strcpy (spec->tmp_copy, TEST_TARGET_FILE);

  spec->size = current_parameters.file_size;
  spec->tmp_data = (uint8_t*)test2_malloc(spec->size);

  for (i = 0; i < spec->size; i++)
    {
      spec->tmp_data[i] = mt_random(&spec->mt);
    }

  return 0;
}

int
random_file_spec_write (random_file_spec *spec) 
{
  int ret;
  if ((ret = main_file_open (&spec->file, spec->tmp_copy, XO_WRITE)) ||
      (ret = main_file_write (&spec->file, spec->tmp_data, spec->size,
			      "write failed")) ||
      (ret = main_file_close (&spec->file)))
    {
      return ret;
    }

  return 0;
}

int
random_file_spec_mutate (random_file_spec *from, random_file_spec *to)
{
  to->seed = mt_random (&static_mtrand);
  mt_init (&to->mt, to->seed);
  main_file_init (&spec->file);

  test_setup();
  spec->tmp_copy = test2_malloc(strlen(TEST_TARGET_FILE) + 1);
  strcpy (spec->tmp_copy, TEST_TARGET_FILE);

  spec->size = current_parameters.file_size;
  spec->tmp_data = (uint8_t*)test2_malloc(spec->size);


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
#endif
