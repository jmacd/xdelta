/* -*- Mode: C++ -*-  */
#include "test.h"

// Declare constants (needed for reference-values, etc).
const xoff_t Constants::BLOCK_SIZE;

void TestRandomNumbers() {
  MTRandom rand;
  int rounds = 1<<20;
  uint64_t usum = 0;
  uint64_t esum = 0;

  for (int i = 0; i < rounds; i++) {
    usum += rand.Rand32();
    esum += rand.ExpRand32(1024);
  }
  
  double allowed_error = 0.001;

  uint32_t umean = usum / rounds;
  uint32_t emean = esum / rounds;

  uint32_t uexpect = UINT32_MAX / 2;
  uint32_t eexpect = 1024;

  if (umean < uexpect * (1.0 - allowed_error) ||
      umean > uexpect * (1.0 + allowed_error)) {
    cerr << "uniform mean error: " << umean << " != " << uexpect << endl;
    abort();
  }

  if (emean < eexpect * (1.0 - allowed_error) ||
      emean > eexpect * (1.0 + allowed_error)) {
    cerr << "exponential mean error: " << emean << " != " << eexpect << endl;
    abort();
  }
}

void TestRandomFile() {
  MTRandom rand1;
  FileSpec spec1(&rand1);

  spec1.GenerateFixedSize(0);
  CHECK_EQ(0, spec1.Size());
  CHECK_EQ(0, spec1.Segments());
  CHECK_EQ(0, spec1.Blocks());

  spec1.GenerateFixedSize(1);
  CHECK_EQ(1, spec1.Size());
  CHECK_EQ(1, spec1.Segments());
  CHECK_EQ(1, spec1.Blocks());

  spec1.GenerateFixedSize(Constants::BLOCK_SIZE);
  CHECK_EQ(Constants::BLOCK_SIZE, spec1.Size());
  CHECK_EQ(1, spec1.Segments());
  CHECK_EQ(1, spec1.Blocks());

  spec1.GenerateFixedSize(Constants::BLOCK_SIZE + 1);
  CHECK_EQ(Constants::BLOCK_SIZE + 1, spec1.Size());
  CHECK_EQ(2, spec1.Segments());
  CHECK_EQ(2, spec1.Blocks());

  spec1.GenerateFixedSize(Constants::BLOCK_SIZE * 2);
  CHECK_EQ(Constants::BLOCK_SIZE * 2, spec1.Size());
  CHECK_EQ(2, spec1.Segments());
  CHECK_EQ(2, spec1.Blocks());
}

void TestFirstByte() {
  MTRandom rand;
  FileSpec spec0(&rand);
  FileSpec spec1(&rand);
  spec0.GenerateFixedSize(0);
  spec1.GenerateFixedSize(1);
  CHECK_EQ(0, CmpDifferentBytes(spec0, spec0));
  CHECK_EQ(0, CmpDifferentBytes(spec1, spec1));
  CHECK_EQ(1, CmpDifferentBytes(spec0, spec1));
  CHECK_EQ(1, CmpDifferentBytes(spec1, spec0));

  spec0.GenerateFixedSize(1);
  spec0.ModifyTo<Modify1stByte>(&spec1);
  CHECK_EQ(1, CmpDifferentBytes(spec0, spec1));

  spec0.GenerateFixedSize(Constants::BLOCK_SIZE + 1);
  spec0.ModifyTo<Modify1stByte>(&spec1);
  CHECK_EQ(1, CmpDifferentBytes(spec0, spec1));
}

int main(int argc, char **argv) {
#define TEST(x) cerr << #x << "..." << endl; x()
  TEST(TestRandomNumbers);
  TEST(TestRandomFile);
  TEST(TestFirstByte);
  return 0;
}

#if 0
static const random_parameters test_parameters[] = {
  { 16384, 4096, 16, 0 },
  { 16384, 4096, 0, 16 },
  { 16384, 4096, 16, 16 },
  { 16384, 4096, 128, 128 },
};

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
