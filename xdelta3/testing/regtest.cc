/* -*- Mode: C++ -*-  */
#include "test.h"

// Declare constants (needed for reference-values, etc).
const xoff_t Constants::BLOCK_SIZE;

// TODO: more options!
void InMemoryEncodeDecode(FileSpec &source_file, FileSpec &target_file) {
  xd3_stream encode_stream;
  xd3_config encode_config;
  xd3_source encode_source;

  xd3_stream decode_stream;
  xd3_config decode_config;
  xd3_source decode_source;

  memset(&encode_stream, 0, sizeof (encode_stream));
  memset(&encode_source, 0, sizeof (encode_source));

  memset(&decode_stream, 0, sizeof (decode_stream));
  memset(&decode_source, 0, sizeof (decode_source));

  xd3_init_config(&encode_config, XD3_ADLER32);
  xd3_init_config(&decode_config, XD3_ADLER32);

  encode_config.winsize = Constants::BLOCK_SIZE;

  CHECK_EQ(0, xd3_config_stream (&encode_stream, &encode_config));
  CHECK_EQ(0, xd3_config_stream (&decode_stream, &decode_config));

  encode_source.size = source_file.Size();
  encode_source.blksize = Constants::BLOCK_SIZE;
  encode_source.curblkno = -1;

  decode_source.size = source_file.Size();
  decode_source.blksize = Constants::BLOCK_SIZE;
  decode_source.curblkno = -1;

  xd3_set_source (&encode_stream, &encode_source);
  xd3_set_source (&decode_stream, &decode_source);

  BlockIterator source_iterator(source_file);
  BlockIterator target_iterator(target_file);
  Block encode_source_block, decode_source_block;
  Block decoded_block, target_block;
  bool encoding = true;
  bool done = false;

  DP(RINT "source %"Q"u target %"Q"u\n",
     source_file.Size(), target_file.Size());

  while (!done) {
    target_iterator.Get(&target_block);

    if (target_block.Size() < target_iterator.BlockSize()) {
      xd3_set_flags(&encode_stream, XD3_FLUSH | encode_stream.flags);
    }

    xd3_avail_input(&encode_stream, target_block.Data(), target_block.Size());

  process:
    int ret;
    if (encoding) {
      ret = xd3_encode_input(&encode_stream);
    } else {
      ret = xd3_decode_input(&decode_stream);
    }

    //DP(RINT "%s = %s\n", encoding ? "encoding" : "decoding",
    //   xd3_strerror(ret));

    switch (ret) {
    case XD3_OUTPUT:
      if (encoding) {
	xd3_avail_input(&decode_stream, 
			encode_stream.next_out, 
			encode_stream.avail_out);
	xd3_consume_output(&encode_stream);
	encoding = false;
      } else {
	decoded_block.Append(decode_stream.next_out,
			     decode_stream.avail_out);
	xd3_consume_output(&decode_stream);
      }
      goto process;

    case XD3_GETSRCBLK: {
      xd3_source *src = (encoding ? &encode_source : &decode_source);
      Block *block = (encoding ? &encode_source_block : &decode_source_block);
      
      source_iterator.SetBlock(src->getblkno);
      source_iterator.Get(block);
      src->curblkno = src->getblkno;
      src->onblk = block->Size();
      src->curblk = block->Data();

      goto process;
    }

    case XD3_INPUT:
      if (!encoding) {
	encoding = true;
	goto process;
      } else {
	if (target_block.Size() < target_iterator.BlockSize()) {
	  done = true;
	} else {
	  target_iterator.Next();
	}
	continue;
      }

    case XD3_WINFINISH:
      if (encoding) {
	encoding = false;
      } else {
	CHECK_EQ(0, CmpDifferentBlockBytes(decoded_block, target_block));
	//DP(RINT "verified block %"Q"u\n", target_iterator.Blkno());
	decoded_block.Reset();
	encoding = true;
      }
      goto process;

    case XD3_WINSTART:
    case XD3_GOTHEADER:
      goto process;

    default:
      CHECK_EQ(0, ret);
      CHECK_EQ(-1, ret);
    }
  }

  xd3_close_stream(&encode_stream);
  xd3_free_stream(&decode_stream);
}

//////////////////////////////////////////////////////////////////////

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
  BlockIterator bi(spec1);

  spec1.GenerateFixedSize(0);
  CHECK_EQ(0, spec1.Size());
  CHECK_EQ(0, spec1.Segments());
  CHECK_EQ(0, spec1.Blocks());
  bi.SetBlock(0);
  CHECK_EQ(0, bi.BytesOnBlock());

  spec1.GenerateFixedSize(1);
  CHECK_EQ(1, spec1.Size());
  CHECK_EQ(1, spec1.Segments());
  CHECK_EQ(1, spec1.Blocks());
  bi.SetBlock(0);
  CHECK_EQ(1, bi.BytesOnBlock());

  spec1.GenerateFixedSize(Constants::BLOCK_SIZE);
  CHECK_EQ(Constants::BLOCK_SIZE, spec1.Size());
  CHECK_EQ(1, spec1.Segments());
  CHECK_EQ(1, spec1.Blocks());
  bi.SetBlock(0);
  CHECK_EQ(Constants::BLOCK_SIZE, bi.BytesOnBlock());
  bi.SetBlock(1);
  CHECK_EQ(0, bi.BytesOnBlock());

  spec1.GenerateFixedSize(Constants::BLOCK_SIZE + 1);
  CHECK_EQ(Constants::BLOCK_SIZE + 1, spec1.Size());
  CHECK_EQ(2, spec1.Segments());
  CHECK_EQ(2, spec1.Blocks());
  bi.SetBlock(0);
  CHECK_EQ(Constants::BLOCK_SIZE, bi.BytesOnBlock());
  bi.SetBlock(1);
  CHECK_EQ(1, bi.BytesOnBlock());

  spec1.GenerateFixedSize(Constants::BLOCK_SIZE * 2);
  CHECK_EQ(Constants::BLOCK_SIZE * 2, spec1.Size());
  CHECK_EQ(2, spec1.Segments());
  CHECK_EQ(2, spec1.Blocks());
  bi.SetBlock(0);
  CHECK_EQ(Constants::BLOCK_SIZE, bi.BytesOnBlock());
  bi.SetBlock(1);
  CHECK_EQ(Constants::BLOCK_SIZE, bi.BytesOnBlock());
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
  spec0.ModifyTo(Modify1stByte(), &spec1);
  CHECK_EQ(1, CmpDifferentBytes(spec0, spec1));

  spec0.GenerateFixedSize(Constants::BLOCK_SIZE + 1);
  spec0.ModifyTo(Modify1stByte(), &spec1);
  CHECK_EQ(1, CmpDifferentBytes(spec0, spec1));

  SizeIterator<size_t, SmallSizes> si(&rand, 20);

  for (; !si.Done(); si.Next()) {
    size_t size = si.Get();
    if (size == 0) {
      continue;
    }
    spec0.GenerateFixedSize(size);
    spec0.ModifyTo(Modify1stByte(), &spec1);
    InMemoryEncodeDecode(spec0, spec1);
  }
}

void TestModifyMutator() {
  MTRandom rand;
  FileSpec spec0(&rand);
  FileSpec spec1(&rand);

  spec0.GenerateFixedSize(Constants::BLOCK_SIZE * 3);

  struct {
    size_t size;
    size_t addr;
  } test_cases[] = {
    { Constants::BLOCK_SIZE, 0 },
    { Constants::BLOCK_SIZE / 2, 1 },
    { Constants::BLOCK_SIZE, 1 },
    { Constants::BLOCK_SIZE * 2, 1 },
  };

  for (size_t i = 0; i < SIZEOF_ARRAY(test_cases); i++) {
    ChangeList cl1;
    cl1.push_back(Change(Change::MODIFY, test_cases[i].size, test_cases[i].addr));
    spec0.ModifyTo(ChangeListMutator(cl1), &spec1);
    
    size_t diff = CmpDifferentBytes(spec0, spec1);
    CHECK_LE(diff, test_cases[i].size);
    CHECK_GE(diff, test_cases[i].size - (2 * test_cases[i].size / 256));

    InMemoryEncodeDecode(spec0, spec1);
  }
}

void TestAddMutator() {
  MTRandom rand;
  FileSpec spec0(&rand);
  FileSpec spec1(&rand);

  spec0.GenerateFixedSize(Constants::BLOCK_SIZE * 2);

  struct {
    size_t size;
    size_t addr;
  } test_cases[] = {
    { 1, 0 },
    { 1, 1 },
    { 1, Constants::BLOCK_SIZE - 1 },
    { 1, Constants::BLOCK_SIZE },
    { 1, Constants::BLOCK_SIZE + 1},
    { 1, 2 * Constants::BLOCK_SIZE },
  };

  for (size_t i = 0; i < SIZEOF_ARRAY(test_cases); i++) {
    ChangeList cl1;
    cl1.push_back(Change(Change::ADD, test_cases[i].size, test_cases[i].addr));
    spec0.Print();
    spec0.ModifyTo(ChangeListMutator(cl1), &spec1);
    spec1.Print();
    
    InMemoryEncodeDecode(spec0, spec1);
  }
}

int main(int argc, char **argv) {
#define TEST(x) cerr << #x << "..." << endl; x()
  TEST(TestRandomNumbers);
  TEST(TestRandomFile);
  TEST(TestFirstByte);
  TEST(TestModifyMutator);
  TEST(TestAddMutator);
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
