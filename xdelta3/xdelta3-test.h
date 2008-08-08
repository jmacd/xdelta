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

/* This is public-domain Mersenne Twister code,
 * attributed to Michael Brundage.  Thanks!
 * http://www.qbrundage.com/michaelb/pubs/essays/random_number_generation.html
 */
static const uint32_t TEST_SEED1 = 5489UL;
#define MT_LEN 624
#define MT_IA 397
static const uint32_t UPPER_MASK = 0x80000000;
static const uint32_t LOWER_MASK = 0x7FFFFFFF;
static const uint32_t MATRIX_A = 0x9908B0DF;

typedef struct mtrand mtrand;

struct mtrand {
  int mt_index_;
  uint32_t mt_buffer_[MT_LEN];
};

void mt_init(mtrand *mt, uint32_t seed) {
  int i;
  mt->mt_buffer_[0] = seed;
  mt->mt_index_ = MT_LEN;
  for (i = 1; i < MT_LEN; i++) {
    /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
    /* In the previous versions, MSBs of the seed affect   */
    /* only MSBs of the array mt[].                        */
    /* 2002/01/09 modified by Makoto Matsumoto             */
    mt->mt_buffer_[i] = 
	(1812433253UL * (mt->mt_buffer_[i-1] ^ (mt->mt_buffer_[i-1] >> 30)) + i);
  }
}


uint32_t mt_random (mtrand *mt) {
  uint32_t y;
  unsigned long mag01[2];
  mag01[0] = 0;
  mag01[1] = MATRIX_A;

  if (mt->mt_index_ >= MT_LEN) {
    int kk;

    for (kk = 0; kk < MT_LEN - MT_IA; kk++) {
      y = (mt->mt_buffer_[kk] & UPPER_MASK) | (mt->mt_buffer_[kk + 1] & LOWER_MASK);
      mt->mt_buffer_[kk] = mt->mt_buffer_[kk + MT_IA] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    for (;kk < MT_LEN - 1; kk++) {
      y = (mt->mt_buffer_[kk] & UPPER_MASK) | (mt->mt_buffer_[kk + 1] & LOWER_MASK);
      mt->mt_buffer_[kk] = mt->mt_buffer_[kk + (MT_IA - MT_LEN)] ^ (y >> 1) ^ mag01[y & 0x1UL];
    }
    y = (mt->mt_buffer_[MT_LEN - 1] & UPPER_MASK) | (mt->mt_buffer_[0] & LOWER_MASK);
    mt->mt_buffer_[MT_LEN - 1] = mt->mt_buffer_[MT_IA - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];
    mt->mt_index_ = 0;
  }
  
  y = mt->mt_buffer_[mt->mt_index_++];
  
  y ^= (y >> 11);
  y ^= (y << 7) & 0x9d2c5680UL;
  y ^= (y << 15) & 0xefc60000UL;
  y ^= (y >> 18);
  
  return y;
}

static mtrand static_mtrand;

#include <math.h>

static uint32_t
mt_exp_rand (uint32_t mean, uint32_t max_value)
{
  double mean_d = mean;
  double erand  = log (1.0 / (mt_random (&static_mtrand) / 
			      (double)UINT32_MAX));
  uint32_t x = (uint32_t) (mean_d * erand + 0.5);

  return min (x, max_value);
}

#ifndef WIN32
#include <sys/wait.h>
#endif

#define MSG_IS(x) (stream->msg != NULL && strcmp ((x), stream->msg) == 0)

static const usize_t TWO_MEGS_AND_DELTA = (2 << 20) + (1 << 10);
static const usize_t ADDR_CACHE_ROUNDS = 10000;

static const usize_t TEST_FILE_MEAN   = 16384;
static const double TEST_ADD_MEAN     = 128;
static const double TEST_ADD_MAX      = 512;
static const double TEST_ADD_RATIO    = 0.1;
static const double TEST_EPSILON      = 0.25;

#define TESTBUFSIZE (1024 * 16)

#define TESTFILESIZE (1024)

static char   TEST_TARGET_FILE[TESTFILESIZE];
static char   TEST_SOURCE_FILE[TESTFILESIZE];
static char   TEST_DELTA_FILE[TESTFILESIZE];
static char   TEST_RECON_FILE[TESTFILESIZE];
static char   TEST_RECON2_FILE[TESTFILESIZE];
static char   TEST_COPY_FILE[TESTFILESIZE];
static char   TEST_NOPERM_FILE[TESTFILESIZE];

#define CHECK(cond) if (!(cond)) { DP(RINT "check failure: " #cond); abort(); }

/* Use a fixed soft config so that test values are fixed.  See also
 * test_compress_text(). */
static const char* test_softcfg_str = "-C9,3,4,8,2,36,70";

static int test_setup (void);

/***********************************************************************
 TEST HELPERS
 ***********************************************************************/

static void DOT (void) { DP(RINT "."); }
static int do_cmd (xd3_stream *stream, const char *buf)
{
  int ret;
  if ((ret = system (buf)) != 0)
    {
      if (WIFEXITED (ret))
	{
	  stream->msg = "command exited non-zero";
	}
      else
	{
	  stream->msg = "abnormal command termination";
	}
      return XD3_INTERNAL;
    }
  return 0;
}

static int do_fail (xd3_stream *stream, const char *buf)
{
  int ret;
  ret = system (buf);
  if (! WIFEXITED (ret) || WEXITSTATUS (ret) != 1)
    {
      stream->msg = "command should have not succeeded";
      DP(RINT "command was %s", buf);
      return XD3_INTERNAL;
    }
  return 0;
}

/* Test that the exponential distribution actually produces its mean. */
static int
test_random_numbers (xd3_stream *stream, int ignore)
{
  usize_t i;
  usize_t sum = 0;
  usize_t mean = 50;
  usize_t n_rounds = 1000000;
  double average, error;
  double allowed_error = 0.1;

  mt_init (& static_mtrand, 0x9f73f7fe);

  for (i = 0; i < n_rounds; i += 1)
    {
      sum += mt_exp_rand (mean, USIZE_T_MAX);
    }

  average = (double) sum / (double) n_rounds;
  error   = average - (double) mean;

  if (error < allowed_error && error > -allowed_error)
    {
      return 0;
    }

  /*DP(RINT "error is %f\n", error);*/
  stream->msg = "random distribution looks broken";
  return XD3_INTERNAL;
}

static void
test_unlink (char* file)
{
  char buf[TESTBUFSIZE];
  while (unlink (file) != 0)
    {
      if (errno == ENOENT)
	{
	  break;
	}
      sprintf (buf, "rm -f %s", file);
      system (buf);
    }
}

static void
test_cleanup (void)
{
  test_unlink (TEST_TARGET_FILE);
  test_unlink (TEST_SOURCE_FILE);
  test_unlink (TEST_DELTA_FILE);
  test_unlink (TEST_RECON_FILE);
  test_unlink (TEST_RECON2_FILE);
  test_unlink (TEST_COPY_FILE);
  test_unlink (TEST_NOPERM_FILE);
}

static int
test_setup (void)
{
  static int x = 0;
  x++;
  sprintf (TEST_TARGET_FILE, "/tmp/xdtest.target.%d", x);
  sprintf (TEST_SOURCE_FILE, "/tmp/xdtest.source.%d", x);
  sprintf (TEST_DELTA_FILE, "/tmp/xdtest.delta.%d", x);
  sprintf (TEST_RECON_FILE, "/tmp/xdtest.recon.%d", x);
  sprintf (TEST_RECON2_FILE, "/tmp/xdtest.recon2.%d", x);
  sprintf (TEST_COPY_FILE, "/tmp/xdtest.copy.%d", x);
  sprintf (TEST_NOPERM_FILE, "/tmp/xdtest.noperm.%d", x);
  test_cleanup();
  return 0;
}

static int
test_make_inputs (xd3_stream *stream, xoff_t *ss_out, xoff_t *ts_out)
{
  usize_t ts = (mt_random (&static_mtrand) % TEST_FILE_MEAN) + TEST_FILE_MEAN / 2;
  usize_t ss = (mt_random (&static_mtrand) % TEST_FILE_MEAN) + TEST_FILE_MEAN / 2;
  uint8_t *buf = (uint8_t*) malloc (ts + ss), *sbuf = buf, *tbuf = buf + ss;
  usize_t sadd = 0, sadd_max = ss * TEST_ADD_RATIO;
  FILE  *tf = NULL, *sf = NULL;
  usize_t i, j;
  int ret;

  if (buf == NULL) { return ENOMEM; }

  if ((tf = fopen (TEST_TARGET_FILE, "w")) == NULL ||
      (ss_out != NULL && (sf = fopen (TEST_SOURCE_FILE, "w")) == NULL))
    {
      stream->msg = "write failed";
      ret = get_errno ();
      goto failure;
    }

  if (ss_out != NULL)
    {
      for (i = 0; i < ss; )
	{
	  sbuf[i++] = mt_random (&static_mtrand);
	}
    }

  /* Then modify the data to produce copies, everything not copied is
   * an add.  The following logic produces the TEST_ADD_RATIO.  The
   * variable SADD contains the number of adds so far, which should
   * not exceed SADD_MAX. */

  /* DP(RINT "ss = %u ts = %u\n", ss, ts); */
  for (i = 0; i < ts; )
    {
      size_t left = ts - i;
      size_t next = mt_exp_rand (TEST_ADD_MEAN, TEST_ADD_MAX);
      size_t add_left = sadd_max - sadd;
      double add_prob = (left == 0) ? 0 : (add_left / (double) left);
      int do_copy;

      next = min (left, next);
      do_copy = (next > add_left || (mt_random (&static_mtrand) / (double)USIZE_T_MAX) >= add_prob);

      if (ss_out == NULL)
	{
	  do_copy &= (i > 0);
	}
      else
	{
	  do_copy &= (ss - next) > 0;
	}

      if (do_copy)
	{
	  /* Copy */
	  size_t offset = mt_random (&static_mtrand) % ((ss_out == NULL) ? i : (ss - next));
	  /* DP(RINT "[%u] copy %u at %u ", i, next, offset); */

	  for (j = 0; j < next; j += 1)
	    {
	      char c = ((ss_out == NULL) ? tbuf : sbuf)[offset + j];
	      /* DP(RINT "%x%x", (c >> 4) & 0xf, c & 0xf); */
	      tbuf[i++] = c;
	    }
	  /* DP(RINT "\n"); */
	}
      else
	{
	  /* Add */
	  /* DP(RINT "[%u] add %u ", i, next); */
	  for (j = 0; j < next; j += 1)
	    {
	      char c = mt_random (&static_mtrand);
	      /* DP(RINT "%x%x", (c >> 4) & 0xf, c & 0xf); */
	      tbuf[i++] = c;
	    }
	  /* DP(RINT "\n"); */
	  sadd += next;
	}
    }

  /* DP(RINT "sadd = %u max = %u\n", sadd, sadd_max); */

  if ((fwrite (tbuf, 1, ts, tf) != ts) ||
      (ss_out != NULL && (fwrite (sbuf, 1, ss, sf) != ss)))
    {
      stream->msg = "write failed";
      ret = get_errno ();
      goto failure;
    }

  if ((ret = fclose (tf)) || (ss_out != NULL && (ret = fclose (sf))))
    {
      stream->msg = "close failed";
      ret = get_errno ();
      goto failure;
    }

  if (ts_out) { (*ts_out) = ts; }
  if (ss_out) { (*ss_out) = ss; }

 failure:
  free (buf);
  return ret;
}

static int
compare_files (xd3_stream *stream, const char* tgt, const char *rec)
{
  FILE *orig, *recons;
  static uint8_t obuf[TESTBUFSIZE], rbuf[TESTBUFSIZE];
  int offset = 0;
  int i;
  int oc, rc;

  if ((orig = fopen (tgt, "r")) == NULL)
    {
      DP(RINT "open %s failed", tgt);
      stream->msg = "open failed";
      return get_errno ();
    }

    if ((recons = fopen (rec, "r")) == NULL)
      {
	DP(RINT "open %s failed", rec);
	stream->msg = "open failed";
	return get_errno ();
      }

  for (;;)
    {
      oc = fread (obuf, 1, TESTBUFSIZE, orig);
      rc = fread (rbuf, 1, TESTBUFSIZE, recons);

      if (oc < 0 || rc < 0)
	{
	  stream->msg = "read failed";
	  return get_errno ();
	}

	if (oc != rc)
	  {
	    stream->msg = "compare files: different length";
	    return XD3_INTERNAL;
	  }

	if (oc == 0)
	  {
	    break;
	  }

	for (i = 0; i < oc; i += 1)
	  {
	    if (obuf[i] != rbuf[i])
	      {
		stream->msg = "compare files: different values";
		return XD3_INTERNAL;
	      }
	  }

	offset += oc;
    }

    fclose (orig);
    fclose (recons);
    return 0;
}

static int
test_save_copy (const char *origname)
{
  char buf[TESTBUFSIZE];
  int ret;

  sprintf (buf, "cp -f %s %s", origname, TEST_COPY_FILE);

  if ((ret = system (buf)) != 0)
    {
      return XD3_INTERNAL;
    }

  return 0;
}

static int
test_file_size (const char* file, xoff_t *size)
{
  struct stat sbuf;
  int ret;
  (*size) = 0;

  if (stat (file, & sbuf) < 0)
    {
      ret = get_errno ();
      DP(RINT "xdelta3: stat failed: %s: %s\n", file, strerror (ret));
      return ret;
    }

  if (! S_ISREG (sbuf.st_mode))
    {
      ret = XD3_INTERNAL;
      DP(RINT "xdelta3: not a regular file: %s: %s\n", file, strerror (ret));
      return ret;
    }

  (*size) = sbuf.st_size;
  return 0;
}

/***********************************************************************
 READ OFFSET
 ***********************************************************************/

/* Common test for read_integer errors: encodes a 64-bit value and
 * then attempts to read as a 32-bit value.  If TRUNC is non-zero,
 * attempts to get errors by shortening the input, otherwise it should
 * overflow.  Expects XD3_INTERNAL and MSG. */
static int
test_read_integer_error (xd3_stream *stream, usize_t trunto, const char *msg)
{
  uint64_t eval = 1ULL << 34;
  uint32_t rval;
  xd3_output *buf = NULL;
  const uint8_t *max;
  const uint8_t *inp;
  int ret;

  buf = xd3_alloc_output (stream, buf);

  if ((ret = xd3_emit_uint64_t (stream, & buf, eval)))
    {
      goto fail;
    }

 again:

  inp = buf->base;
  max = buf->base + buf->next - trunto;

  if ((ret = xd3_read_uint32_t (stream, & inp, max, & rval)) != XD3_INVALID_INPUT ||
      !MSG_IS (msg))
    {
      ret = XD3_INTERNAL;
    }
  else if (trunto && trunto < buf->next)
    {
      trunto += 1;
      goto again;
    }
  else
    {
      ret = 0;
    }

 fail:
  xd3_free_output (stream, buf);
  return ret;
}

/* Test integer overflow using the above routine. */
static int
test_decode_integer_overflow (xd3_stream *stream, int unused)
{
  return test_read_integer_error (stream, 0, "overflow in read_intger");
}

/* Test integer EOI using the above routine. */
static int
test_decode_integer_end_of_input (xd3_stream *stream, int unused)
{
  return test_read_integer_error (stream, 1, "end-of-input in read_integer");
}

/* Test that emit_integer/decode_integer/sizeof_integer/read_integer
 * work on correct inputs.  Tests powers of (2^7), plus or minus, up
 * to the maximum value. */
#define TEST_ENCODE_DECODE_INTEGER(TYPE,ONE,MAX) \
  xd3_output *rbuf = NULL; \
  xd3_output *dbuf = NULL; \
  TYPE values[64]; \
  usize_t nvalues = 0; \
  usize_t i; \
  int ret = 0; \
 \
  for (i = 0; i < (sizeof (TYPE) * 8); i += 7) \
    { \
      values[nvalues++] = (ONE << i) - ONE; \
      values[nvalues++] = (ONE << i); \
      values[nvalues++] = (ONE << i) + ONE; \
    } \
 \
  values[nvalues++] = MAX-ONE; \
  values[nvalues++] = MAX; \
 \
  rbuf = xd3_alloc_output (stream, rbuf); \
  dbuf = xd3_alloc_output (stream, dbuf); \
 \
  for (i = 0; i < nvalues; i += 1) \
    { \
      const uint8_t *max; \
      const uint8_t *inp; \
      TYPE val;			\
 \
      DOT (); \
      rbuf->next = 0; \
 \
      if ((ret = xd3_emit_ ## TYPE (stream, & rbuf, values[i])) || \
	  (ret = xd3_emit_ ## TYPE (stream, & dbuf, values[i]))) \
	{ \
	  goto fail; \
	} \
 \
      inp = rbuf->base; \
      max = rbuf->base + rbuf->next; \
 \
      if (rbuf->next != xd3_sizeof_ ## TYPE (values[i])) \
	{ \
	  ret = XD3_INTERNAL; \
	  goto fail; \
	} \
 \
      if ((ret = xd3_read_ ## TYPE (stream, & inp, max, & val))) \
	{ \
	  goto fail; \
	} \
 \
      if (val != values[i]) \
	{ \
	  ret = XD3_INTERNAL; \
	  goto fail; \
	} \
 \
      DOT (); \
    } \
 \
  stream->next_in  = dbuf->base; \
  stream->avail_in = dbuf->next; \
 \
  for (i = 0; i < nvalues; i += 1) \
    { \
      TYPE val; \
 \
      if ((ret = xd3_decode_ ## TYPE (stream, & val))) \
        { \
          goto fail; \
        } \
 \
      if (val != values[i]) \
        { \
          ret = XD3_INTERNAL; \
          goto fail; \
        } \
    } \
 \
  if (stream->avail_in != 0) \
    { \
      ret = XD3_INTERNAL; \
      goto fail; \
    } \
 \
 fail: \
  xd3_free_output (stream, rbuf); \
  xd3_free_output (stream, dbuf); \
 \
  return ret

static int
test_encode_decode_uint32_t (xd3_stream *stream, int unused)
{
  TEST_ENCODE_DECODE_INTEGER(uint32_t,1U,UINT32_MAX);
}

static int
test_encode_decode_uint64_t (xd3_stream *stream, int unused)
{
  TEST_ENCODE_DECODE_INTEGER(uint64_t,1ULL,UINT64_MAX);
}

static int
test_usize_t_overflow (xd3_stream *stream, int unused)
{
  if (USIZE_T_OVERFLOW (0, 0)) { goto fail; }
  if (USIZE_T_OVERFLOW (USIZE_T_MAX, 0)) { goto fail; }
  if (USIZE_T_OVERFLOW (0, USIZE_T_MAX)) { goto fail; }
  if (USIZE_T_OVERFLOW (USIZE_T_MAX / 2, 0)) { goto fail; }
  if (USIZE_T_OVERFLOW (USIZE_T_MAX / 2, USIZE_T_MAX / 2)) { goto fail; }
  if (USIZE_T_OVERFLOW (USIZE_T_MAX / 2, USIZE_T_MAX / 2 + 1)) { goto fail; }

  if (! USIZE_T_OVERFLOW (USIZE_T_MAX, 1)) { goto fail; }
  if (! USIZE_T_OVERFLOW (1, USIZE_T_MAX)) { goto fail; }
  if (! USIZE_T_OVERFLOW (USIZE_T_MAX / 2 + 1, USIZE_T_MAX / 2 + 1)) { goto fail; }

  return 0;

 fail:
  stream->msg = "incorrect overflow computation";
  return XD3_INTERNAL;
}

static int
test_forward_match (xd3_stream *stream, int unused)
{
  usize_t i;
  uint8_t buf1[256], buf2[256];

  memset(buf1, 0, 256);
  memset(buf2, 0, 256);

  for (i = 0; i < 256; i++)
    {
      CHECK(xd3_forward_match(buf1, buf2, i) == i);
    }

  for (i = 0; i < 255; i++)
    {
      buf2[i] = 1;
      CHECK(xd3_forward_match(buf1, buf2, 256) == i);
      buf2[i] = 0;
    }

  return 0;
}

/***********************************************************************
 Address cache
 ***********************************************************************/

static int
test_address_cache (xd3_stream *stream, int unused)
{
  int ret;
  usize_t i;
  usize_t offset;
  usize_t *addrs;
  uint8_t *big_buf, *buf_max;
  const uint8_t *buf;
  xd3_output *outp;
  uint8_t *modes;
  int mode_counts[16];

  stream->acache.s_near = stream->code_table_desc->near_modes;
  stream->acache.s_same = stream->code_table_desc->same_modes;

  if ((ret = xd3_encode_init_partial (stream))) { return ret; }

  addrs = (usize_t*) xd3_alloc (stream, sizeof (usize_t), ADDR_CACHE_ROUNDS);
  modes = (uint8_t*) xd3_alloc (stream, sizeof (uint8_t), ADDR_CACHE_ROUNDS);

  memset (mode_counts, 0, sizeof (mode_counts));
  memset (modes, 0, ADDR_CACHE_ROUNDS);

  addrs[0] = 0;

  mt_init (& static_mtrand, 0x9f73f7fc);

  /* First pass: encode addresses */
  xd3_init_cache (& stream->acache);

  for (offset = 1; offset < ADDR_CACHE_ROUNDS; offset += 1)
    {
      double p;
      usize_t addr;
      usize_t prev_i;
      usize_t nearby;

      p         = (mt_random (&static_mtrand) / (double)USIZE_T_MAX);
      prev_i    = mt_random (&static_mtrand) % offset;
      nearby    = (mt_random (&static_mtrand) % 256) % offset;
      nearby    = max (1U, nearby);

      if (p < 0.1)      { addr = addrs[offset-nearby]; }
      else if (p < 0.4) { addr = min (addrs[prev_i] + nearby, offset-1); }
      else              { addr = prev_i; }

      if ((ret = xd3_encode_address (stream, addr, offset, & modes[offset]))) { return ret; }

      addrs[offset] = addr;
      mode_counts[modes[offset]] += 1;
    }

  /* Copy addresses into a contiguous buffer. */
  big_buf = (uint8_t*) xd3_alloc (stream, xd3_sizeof_output (ADDR_HEAD (stream)), 1);

  for (offset = 0, outp = ADDR_HEAD (stream); outp != NULL; offset += outp->next, outp = outp->next_page)
    {
      memcpy (big_buf + offset, outp->base, outp->next);
    }

  buf_max = big_buf + offset;
  buf     = big_buf;

  /* Second pass: decode addresses */
  xd3_init_cache (& stream->acache);

  for (offset = 1; offset < ADDR_CACHE_ROUNDS; offset += 1)
    {
      uint32_t addr;

      if ((ret = xd3_decode_address (stream, offset, modes[offset], & buf, buf_max, & addr))) { return ret; }

      if (addr != addrs[offset])
	{
	  stream->msg = "incorrect decoded address";
	  return XD3_INTERNAL;
	}
    }

  /* Check that every byte, mode was used. */
  if (buf != buf_max)
    {
      stream->msg = "address bytes not used";
      return XD3_INTERNAL;
    }

  for (i = 0; i < (2 + stream->acache.s_same + stream->acache.s_near); i += 1)
    {
      if (mode_counts[i] == 0)
	{
	  stream->msg = "address mode not used";
	  return XD3_INTERNAL;
	}
    }

  xd3_free (stream, modes);
  xd3_free (stream, addrs);
  xd3_free (stream, big_buf);

  return 0;
}

/***********************************************************************
 Encode and decode with single bit error
 ***********************************************************************/

/* It compresses from 256 to around 185 bytes.
 * Avoids matching addresses that are a single-bit difference.
 * Avoids matching address 0. */
static const uint8_t test_text[] =
"this is a story\n"
"abouttttttttttt\n"
"- his is a stor\n"
"- about nothing "
" all. boutique -"
"his story is a -"
"about           "
"what happens all"
" the time what -"
"am I ttttttt the"
" person said, so"
" what, per son -"
" gory story is -"
" about nothing -"
"tttttt to test -"
"his sto nothing";

static const uint8_t test_apphead[] = "header test";

static int
test_compress_text (xd3_stream  *stream,
		    uint8_t     *encoded,
		    usize_t     *encoded_size)
{
  int ret;
  xd3_config cfg;
  int oflags = stream->flags;
  int flags = stream->flags | XD3_FLUSH;

  xd3_free_stream (stream);
  xd3_init_config (& cfg, flags);

  /* This configuration is fixed so that the "expected non-error" the counts in
   * decompress_single_bit_errors are too.  See test_coftcfg_str. */
  cfg.smatch_cfg = XD3_SMATCH_SOFT;
  cfg.smatcher_soft.name = "test";
  cfg.smatcher_soft.large_look = 64; /* no source, not used */
  cfg.smatcher_soft.large_step = 64; /* no source, not used */
  cfg.smatcher_soft.small_look = 4;
  cfg.smatcher_soft.small_chain = 128;
  cfg.smatcher_soft.small_lchain = 16;
  cfg.smatcher_soft.max_lazy = 8;
  cfg.smatcher_soft.long_enough = 128;

  xd3_config_stream (stream, & cfg);

  (*encoded_size) = 0;

  xd3_set_appheader (stream, test_apphead, strlen ((char*) test_apphead));

  if ((ret = xd3_encode_stream (stream, test_text, sizeof (test_text),
				encoded, encoded_size, 4*sizeof (test_text)))) { goto fail; }

  if ((ret = xd3_close_stream (stream))) { goto fail; }

 fail:
  xd3_free_stream (stream);
  xd3_init_config (& cfg, oflags);
  xd3_config_stream (stream, & cfg);
  return ret;
}

static int
test_decompress_text (xd3_stream *stream, uint8_t *enc, usize_t enc_size, usize_t test_desize)
{
  xd3_config cfg;
  char decoded[sizeof (test_text)];
  uint8_t *apphead;
  usize_t apphead_size;
  usize_t decoded_size;
  const char *msg;
  int  ret;
  usize_t pos = 0;
  int flags = stream->flags;
  usize_t take;

 input:
  /* Test decoding test_desize input bytes at a time */
  take = min (enc_size - pos, test_desize);
  CHECK(take > 0);

  xd3_avail_input (stream, enc + pos, take);
 again:
  ret = xd3_decode_input (stream);

  pos += take;
  take = 0;

  switch (ret)
    {
    case XD3_OUTPUT:
      break;
    case XD3_WINSTART:
    case XD3_GOTHEADER:
      goto again;
    case XD3_INPUT:
      if (pos < enc_size) { goto input; }
      /* else fallthrough */
    case XD3_WINFINISH:
    default:
      goto fail;
    }

  CHECK(ret == XD3_OUTPUT);
  CHECK(pos == enc_size);

  if (stream->avail_out != sizeof (test_text))
    {
      stream->msg = "incorrect output size";
      ret = XD3_INTERNAL;
      goto fail;
    }

  decoded_size = stream->avail_out;
  memcpy (decoded, stream->next_out, stream->avail_out);

  xd3_consume_output (stream);

  if ((ret = xd3_get_appheader (stream, & apphead, & apphead_size))) { goto fail; }

  if (apphead_size != strlen ((char*) test_apphead) ||
      memcmp (apphead, test_apphead, strlen ((char*) test_apphead)) != 0)
    {
      stream->msg = "incorrect appheader";
      ret = XD3_INTERNAL;
      goto fail;
    }

  if ((ret = xd3_decode_input (stream)) != XD3_WINFINISH ||
      (ret = xd3_close_stream (stream)) != 0)
    {
      goto fail;
    }

  if (decoded_size != sizeof (test_text) ||
      memcmp (decoded, test_text, sizeof (test_text)) != 0)
    {
      stream->msg = "incorrect output text";
      ret = EIO;
    }

 fail:
  msg = stream->msg;
  xd3_free_stream (stream);
  xd3_init_config (& cfg, flags);
  xd3_config_stream (stream, & cfg);
  stream->msg = msg;

  return ret;
}

static int
test_decompress_single_bit_error (xd3_stream *stream, int expected_non_failures)
{
  int ret;
  usize_t i;
  uint8_t encoded[4*sizeof (test_text)]; /* make room for alt code table */
  usize_t  encoded_size;
  int non_failures = 0;
  int cksum = (stream->flags & XD3_ADLER32) != 0;

//#define DEBUG_TEST_FAILURES
#ifndef DEBUG_TEST_FAILURES
#define TEST_FAILURES()
#else
  /* For checking non-failure cases by hand, enable this macro and run
   * xdelta printdelta with print_cpymode disabled.  Every non-failure
   * should change a copy address mode, which doesn't cause a failure
   * because the address cache starts out with all zeros.

    ./xdelta3 test
    for i in test_text.xz.*; do ./xdelta3 printdelta $i > $i.out;
    diff $i.out test_text.xz.0.out; done

   */
  system ("rm -rf test_text.*");
  {
    char buf[TESTBUFSIZE];
    FILE *f;
    sprintf (buf, "test_text");
    f = fopen (buf, "w");
    fwrite (test_text,1,sizeof (test_text),f);
    fclose (f);
  }
#define TEST_FAILURES()                                         \
  do {                                                          \
    char buf[TESTBUFSIZE];      				\
    FILE *f;                                                    \
    sprintf (buf, "test_text.xz.%d", non_failures);             \
    f = fopen (buf, "w");                                       \
    fwrite (encoded,1,encoded_size,f);                          \
    fclose (f);                                                 \
  } while (0)
#endif

  stream->sec_data.inefficient = 1;
  stream->sec_inst.inefficient = 1;
  stream->sec_addr.inefficient = 1;

  /* Encode text, test correct input */
  if ((ret = test_compress_text (stream, encoded, & encoded_size)))
    {
      /*stream->msg = "without error: encode failure";*/
      return ret;
    }

  if ((ret = test_decompress_text (stream, encoded, encoded_size,
				   sizeof (test_text) / 4)))
    {
      /*stream->msg = "without error: decode failure";*/
      return ret;
    }

  TEST_FAILURES();

  for (i = 0; i < encoded_size*8; i += 1)
    {
      /* Single bit error. */
      encoded[i/8] ^= 1 << (i%8);

      if ((ret = test_decompress_text (stream, encoded,
				       encoded_size, sizeof (test_text))) == 0)
	{
	  non_failures += 1;
#ifdef DEBUG_TEST_FAILURES
	  DP(RINT "%u[%u] non-failure %u\n", i/8, i%8, non_failures);
#endif
	  TEST_FAILURES();
	}
      else
	{
	  /*DP(RINT "%u[%u] failure: %s\n", i/8, i%8, stream->msg);*/
	}

      /* decompress_text returns EIO when the final memcmp() fails, but that
       * should never happen with checksumming on. */
      if (cksum && ret == EIO)
	{
	  /*DP(RINT "%u[%u] cksum mismatch\n", i/8, i%8);*/
	  stream->msg = "checksum mismatch";
	  return XD3_INTERNAL;
	}

      /* Undo single bit error. */
      encoded[i/8] ^= 1 << (i%8);
    }

  /* Test correct input again */
  if ((ret = test_decompress_text (stream, encoded, encoded_size, 1)))
    {
      /*stream->msg = "without error: decode failure";*/
      return ret;
    }

  /* Check expected non-failures */
  if (non_failures != expected_non_failures)
    {
      DP(RINT "non-failures %u; expected %u",
	 non_failures, expected_non_failures);
      stream->msg = "incorrect";
      return XD3_INTERNAL;
    }

  DOT ();

  return 0;
}

/***********************************************************************
 Secondary compression tests
 ***********************************************************************/

#if SECONDARY_ANY
typedef int (*sec_dist_func) (xd3_stream *stream, xd3_output *data);

static int sec_dist_func1 (xd3_stream *stream, xd3_output *data);
static int sec_dist_func2 (xd3_stream *stream, xd3_output *data);
static int sec_dist_func3 (xd3_stream *stream, xd3_output *data);
static int sec_dist_func4 (xd3_stream *stream, xd3_output *data);
static int sec_dist_func5 (xd3_stream *stream, xd3_output *data);
static int sec_dist_func6 (xd3_stream *stream, xd3_output *data);
static int sec_dist_func7 (xd3_stream *stream, xd3_output *data);
static int sec_dist_func8 (xd3_stream *stream, xd3_output *data);
static int sec_dist_func9 (xd3_stream *stream, xd3_output *data);
static int sec_dist_func10 (xd3_stream *stream, xd3_output *data);
static int sec_dist_func11 (xd3_stream *stream, xd3_output *data);

static sec_dist_func sec_dists[] =
{
  sec_dist_func1,
  sec_dist_func2,
  sec_dist_func3,
  sec_dist_func4,
  sec_dist_func5,
  sec_dist_func6,
  sec_dist_func7,
  sec_dist_func8,
  sec_dist_func9,
  sec_dist_func10,
  sec_dist_func11,
};

/* Test ditsribution: 100 bytes of the same character (13). */
static int
sec_dist_func1 (xd3_stream *stream, xd3_output *data)
{
  int i, ret;
  for (i = 0; i < 100; i += 1)
    {
      if ((ret = xd3_emit_byte (stream, & data, 13))) { return ret; }
    }
  return 0;
}

/* Test ditsribution: uniform covering half the alphabet. */
static int
sec_dist_func2 (xd3_stream *stream, xd3_output *data)
{
  int i, ret;
  for (i = 0; i < ALPHABET_SIZE; i += 1)
    {
      if ((ret = xd3_emit_byte (stream, & data, i%(ALPHABET_SIZE/2)))) { return ret; }
    }
  return 0;
}

/* Test ditsribution: uniform covering the entire alphabet. */
static int
sec_dist_func3 (xd3_stream *stream, xd3_output *data)
{
  int i, ret;
  for (i = 0; i < ALPHABET_SIZE; i += 1)
    {
      if ((ret = xd3_emit_byte (stream, & data, i%ALPHABET_SIZE))) { return ret; }
    }
  return 0;
}

/* Test distribution: An exponential distribution covering half the alphabet */
static int
sec_dist_func4 (xd3_stream *stream, xd3_output *data)
{
  int i, ret, x;
  for (i = 0; i < ALPHABET_SIZE*20; i += 1)
    {
      x = mt_exp_rand (10, ALPHABET_SIZE/2);
      if ((ret = xd3_emit_byte (stream, & data, x))) { return ret; }
    }
  return 0;
}

/* Test distribution: An exponential distribution covering the entire alphabet */
static int
sec_dist_func5 (xd3_stream *stream, xd3_output *data)
{
  int i, ret, x;
  for (i = 0; i < ALPHABET_SIZE*20; i += 1)
    {
      x = mt_exp_rand (10, ALPHABET_SIZE-1);
      if ((ret = xd3_emit_byte (stream, & data, x))) { return ret; }
    }
  return 0;
}

/* Test distribution: An uniform random distribution covering half the alphabet */
static int
sec_dist_func6 (xd3_stream *stream, xd3_output *data)
{
  int i, ret, x;
  for (i = 0; i < ALPHABET_SIZE*20; i += 1)
    {
      x = mt_random (&static_mtrand) % (ALPHABET_SIZE/2);
      if ((ret = xd3_emit_byte (stream, & data, x))) { return ret; }
    }
  return 0;
}

/* Test distribution: An uniform random distribution covering the entire alphabet */
static int
sec_dist_func7 (xd3_stream *stream, xd3_output *data)
{
  int i, ret, x;
  for (i = 0; i < ALPHABET_SIZE*20; i += 1)
    {
      x = mt_random (&static_mtrand) % ALPHABET_SIZE;
      if ((ret = xd3_emit_byte (stream, & data, x))) { return ret; }
    }
  return 0;
}

/* Test distribution: A small number of frequent characters, difficult
 * to divide into many groups */
static int
sec_dist_func8 (xd3_stream *stream, xd3_output *data)
{
  int i, ret;
  for (i = 0; i < ALPHABET_SIZE*5; i += 1)
    {
      if ((ret = xd3_emit_byte (stream, & data, 0))) { return ret; }
      if ((ret = xd3_emit_byte (stream, & data, 64))) { return ret; }
      if ((ret = xd3_emit_byte (stream, & data, 128))) { return ret; }
      if ((ret = xd3_emit_byte (stream, & data, 255))) { return ret; }
    }
  return 0;
}

/* Test distribution: One that causes many FGK block promotions (found a bug) */
static int
sec_dist_func9 (xd3_stream *stream, xd3_output *data)
{
  int i, ret;

  int ramp   = 0;
  int rcount = 0;
  int prom   = 0;
  int pcount = 0;

  /* 200 was long enough to trigger it--only when stricter checking
   * that counted all blocks was turned on, but it seems I deleted
   * this code. (missing fgk_free_block on line 398). */
  for (i = 0; i < ALPHABET_SIZE*200; i += 1)
    {
    repeat:
      if (ramp < ALPHABET_SIZE)
	{
	  /* Initially Nth symbol has (N+1) frequency */
	  if (rcount <= ramp)
	    {
	      rcount += 1;
	      if ((ret = xd3_emit_byte (stream, & data, ramp))) { return ret; }
	      continue;
	    }

	  ramp   += 1;
	  rcount  = 0;
	  goto repeat;
	}

      /* Thereafter, promote least freq to max freq */
      if (pcount == ALPHABET_SIZE)
	{
	  pcount = 0;
	  prom   = (prom + 1) % ALPHABET_SIZE;
	}

      pcount += 1;
      if ((ret = xd3_emit_byte (stream, & data, prom))) { return ret; }
    }

  return 0;
}

/* Test distribution: freq[i] == i*i, creates a 21-bit code length, fixed in 3.0r. */
static int
sec_dist_func10 (xd3_stream *stream, xd3_output *data)
{
  int i, j, ret;
  for (i = 0; i < ALPHABET_SIZE; i += 1)
    {
      for (j = 0; j <= (i*i); j += 1)
	{
	  if ((ret = xd3_emit_byte (stream, & data, i))) { return ret; }
	}
    }
  return 0;
}

/* Test distribution: fibonacci */
static int
sec_dist_func11 (xd3_stream *stream, xd3_output *data)
{
  int sum0 = 0;
  int sum1 = 1;
  int i, j, ret;
  for (i = 0; i < 33; ++i)
    {
      for (j = 0; j < (sum0 + sum1); ++j)
	{
	  if ((ret = xd3_emit_byte (stream, & data, i))) { return ret; }
	}
      sum0 = sum1;
      sum1 = j;
    }
  return 0;
}

static int
test_secondary_decode (xd3_stream         *stream,
		       const xd3_sec_type *sec,
		       usize_t              input_size,
		       usize_t              compress_size,
		       const uint8_t      *dec_input,
		       const uint8_t      *dec_correct,
		       uint8_t            *dec_output)
{
  int ret;
  xd3_sec_stream *dec_stream;
  const uint8_t *dec_input_used, *dec_input_end;
  uint8_t *dec_output_used, *dec_output_end;

  if ((dec_stream = sec->alloc (stream)) == NULL) { return ENOMEM; }

  sec->init (dec_stream);

  dec_input_used = dec_input;
  dec_input_end  = dec_input + compress_size;

  dec_output_used = dec_output;
  dec_output_end  = dec_output + input_size;

  if ((ret = sec->decode (stream, dec_stream,
			  & dec_input_used, dec_input_end,
			  & dec_output_used, dec_output_end)))
    {
      goto fail;
    }

  if (dec_input_used != dec_input_end)
    {
      stream->msg = "unused input";
      ret = XD3_INTERNAL;
      goto fail;
    }

  if (dec_output_used != dec_output_end)
    {
      stream->msg = "unfinished output";
      ret = XD3_INTERNAL;
      goto fail;
    }

  if (memcmp (dec_output, dec_correct, input_size) != 0)
    {
      stream->msg = "incorrect output";
      ret = XD3_INTERNAL;
      goto fail;
    }

 fail:
  sec->destroy (stream, dec_stream);
  return ret;
}

static int
test_secondary (xd3_stream *stream, const xd3_sec_type *sec, int groups)
{
  int test_i, ret;
  xd3_output *in_head, *out_head, *p;
  usize_t p_off, input_size, compress_size;
  uint8_t *dec_input = NULL, *dec_output = NULL, *dec_correct = NULL;
  xd3_sec_stream *enc_stream;
  xd3_sec_cfg cfg;

  memset (& cfg, 0, sizeof (cfg));

  cfg.inefficient = 1;

  for (cfg.ngroups = 1; cfg.ngroups <= groups; cfg.ngroups += 1)
    {
      DP(RINT "\n...");
      for (test_i = 0; test_i < SIZEOF_ARRAY (sec_dists); test_i += 1)
	{
	  mt_init (& static_mtrand, 0x9f73f7fc);

	  in_head  = xd3_alloc_output (stream, NULL);
	  out_head = xd3_alloc_output (stream, NULL);
	  enc_stream = sec->alloc (stream);
	  dec_input = NULL;
	  dec_output = NULL;
	  dec_correct = NULL;

	  if (in_head == NULL || out_head == NULL || enc_stream == NULL)
	    {
	      goto nomem;
	    }

	  if ((ret = sec_dists[test_i] (stream, in_head))) { goto fail; }

	  sec->init (enc_stream);

	  /* Encode data */
	  if ((ret = sec->encode (stream, enc_stream,
				  in_head, out_head, & cfg)))
	    {
	      DP(RINT "test %u: encode: %s", test_i, stream->msg);
	      goto fail;
	    }

	  /* Calculate sizes, allocate contiguous arrays for decoding */
	  input_size    = xd3_sizeof_output (in_head);
	  compress_size = xd3_sizeof_output (out_head);

	  DP(RINT "%.3f", 8.0 * (double) compress_size / (double) input_size);

	  if ((dec_input   = xd3_alloc (stream, compress_size, 1)) == NULL ||
	      (dec_output  = xd3_alloc (stream, input_size, 1)) == NULL ||
	      (dec_correct = xd3_alloc (stream, input_size, 1)) == NULL)
	    {
	      goto nomem;
	    }

	  /* Fill the compressed data array */
	  for (p_off = 0, p = out_head; p != NULL;
	       p_off += p->next, p = p->next_page)
	    {
	      memcpy (dec_input + p_off, p->base, p->next);
	    }

	  CHECK(p_off == compress_size);

	  /* Fill the input data array */
	  for (p_off = 0, p = in_head; p != NULL;
	       p_off += p->next, p = p->next_page)
	    {
	      memcpy (dec_correct + p_off, p->base, p->next);
	    }

	  CHECK(p_off == input_size);

	  if ((ret = test_secondary_decode (stream, sec, input_size,
					    compress_size, dec_input,
					    dec_correct, dec_output)))
	    {
	      DP(RINT "test %u: decode: %s", test_i, stream->msg);
	      goto fail;
	    }

	  /* Single-bit error test, only cover the first 10 bytes.
	   * Some non-failures are expected in the Huffman case:
	   * Changing the clclen array, for example, may not harm the
	   * decoding.  Really looking for faults here. */
	  {
	    int i;
	    int bytes = min (compress_size, 10U);
	    for (i = 0; i < bytes * 8; i += 1)
	      {
		dec_input[i/8] ^= 1 << (i%8);

		if ((ret = test_secondary_decode (stream, sec, input_size,
						  compress_size, dec_input,
						  dec_correct, dec_output))
		    == 0)
		  {
		    /*DP(RINT "test %u: decode single-bit [%u/%u]
		      error non-failure", test_i, i/8, i%8);*/
		  }

		dec_input[i/8] ^= 1 << (i%8);

		if ((i % (2*bytes)) == (2*bytes)-1)
		  {
		    DOT ();
		  }
	      }
	    ret = 0;
	  }

	  if (0) { nomem: ret = ENOMEM; }

	fail:
	  sec->destroy (stream, enc_stream);
	  xd3_free_output (stream, in_head);
	  xd3_free_output (stream, out_head);
	  xd3_free (stream, dec_input);
	  xd3_free (stream, dec_output);
	  xd3_free (stream, dec_correct);

	  if (ret != 0) { return ret; }
	}
    }

  return 0;
}

IF_FGK (static int test_secondary_fgk  (xd3_stream *stream, int gp)
	{ return test_secondary (stream, & fgk_sec_type, gp); })
IF_DJW (static int test_secondary_huff (xd3_stream *stream, int gp)
	{ return test_secondary (stream, & djw_sec_type, gp); })
#endif

/***********************************************************************
 TEST INSTRUCTION TABLE
 ***********************************************************************/

/* Test that xd3_choose_instruction() does the right thing for its code
 * table. */
static int
test_choose_instruction (xd3_stream *stream, int ignore)
{
  int i;

  stream->code_table = (*stream->code_table_func) ();

  for (i = 0; i < 256; i += 1)
    {
      const xd3_dinst *d = stream->code_table + i;
      xd3_rinst prev, inst;

      CHECK(d->type1 > 0);

      memset (& prev, 0, sizeof (prev));
      memset (& inst, 0, sizeof (inst));

      if (d->type2 == 0)
	{
	  inst.type = d->type1;

	  if ((inst.size = d->size1) == 0)
	    {
	      inst.size = TESTBUFSIZE;
	    }

	  XD3_CHOOSE_INSTRUCTION (stream, NULL, & inst);

	  if (inst.code2 != 0 || inst.code1 != i)
	    {
	      stream->msg = "wrong single instruction";
	      return XD3_INTERNAL;
	    }
	}
      else
	{
	  prev.type = d->type1;
	  prev.size = d->size1;
	  inst.type = d->type2;
	  inst.size = d->size2;

	  XD3_CHOOSE_INSTRUCTION (stream, & prev, & inst);

	  if (prev.code2 != i)
	    {
	      stream->msg = "wrong double instruction";
	      return XD3_INTERNAL;
	    }
	}
    }

  return 0;
}

/***********************************************************************
 TEST INSTRUCTION TABLE CODING
 ***********************************************************************/

#if GENERIC_ENCODE_TABLES
/* Test that encoding and decoding a code table works */
static int
test_encode_code_table (xd3_stream *stream, int ignore)
{
  int ret;
  const uint8_t *comp_data;
  usize_t comp_size;

  if ((ret = xd3_compute_alternate_table_encoding (stream, & comp_data, & comp_size)))
    {
      return ret;
    }

  stream->acache.s_near = __alternate_code_table_desc.near_modes;
  stream->acache.s_same = __alternate_code_table_desc.same_modes;

  if ((ret = xd3_apply_table_encoding (stream, comp_data, comp_size)))
    {
      return ret;
    }

  if (memcmp (stream->code_table, xd3_alternate_code_table (), sizeof (xd3_dinst) * 256) != 0)
    {
      stream->msg = "wrong code table reconstruction";
      return XD3_INTERNAL;
    }

  return 0;
}
#endif

/***********************************************************************
 64BIT STREAMING
 ***********************************************************************/

/* This test encodes and decodes a series of 1 megabyte windows, each
 * containing a long run of zeros along with a single xoff_t size
 * record to indicate the sequence. */
static int
test_streaming (xd3_stream *in_stream, uint8_t *encbuf, uint8_t *decbuf, uint8_t *delbuf, usize_t megs)
{
  xd3_stream estream, dstream;
  int ret;
  usize_t i, delsize, decsize;

  if ((ret = xd3_config_stream (& estream, NULL)) ||
      (ret = xd3_config_stream (& dstream, NULL)))
    {
      goto fail;
    }

  for (i = 0; i < megs; i += 1)
    {
      ((usize_t*) encbuf)[0] = i;

      if ((i % 200) == 199) { DOT (); }

      if ((ret = xd3_process_stream (1, & estream, xd3_encode_input, 0,
				     encbuf, 1 << 20,
				     delbuf, & delsize, 1 << 10)))
	{
	  in_stream->msg = estream.msg;
	  goto fail;
	}

      if ((ret = xd3_process_stream (0, & dstream, xd3_decode_input, 0,
				     delbuf, delsize,
				     decbuf, & decsize, 1 << 20)))
	{
	  in_stream->msg = dstream.msg;
	  goto fail;
	}

      if (decsize != 1 << 20 ||
	  memcmp (encbuf, decbuf, 1 << 20) != 0)
	{
	  in_stream->msg = "wrong result";
	  ret = XD3_INTERNAL;
	  goto fail;
	}
    }

  if ((ret = xd3_close_stream (& estream)) ||
      (ret = xd3_close_stream (& dstream)))
    {
      goto fail;
    }

 fail:
  xd3_free_stream (& estream);
  xd3_free_stream (& dstream);
  return ret;
}

/* Run tests of data streaming of over and around 4GB of data. */
static int
test_compressed_stream_overflow (xd3_stream *stream, int ignore)
{
  int ret;
  uint8_t *buf;

  if ((buf = (uint8_t*) malloc (TWO_MEGS_AND_DELTA)) == NULL) { return ENOMEM; }

  memset (buf, 0, TWO_MEGS_AND_DELTA);

  /* Test overflow of a 32-bit file offset. */
  if (SIZEOF_XOFF_T == 4)
    {
      ret = test_streaming (stream, buf, buf + (1 << 20), buf + (2 << 20), (1 << 12) + 1);

      if (ret == XD3_INVALID_INPUT && MSG_IS ("decoder file offset overflow"))
	{
	  ret = 0;
	}
      else
	{
          XPR(NT XD3_LIB_ERRMSG (stream, ret));
	  stream->msg = "expected overflow condition";
	  ret = XD3_INTERNAL;
	  goto fail;
	}
    }

  /* Test transfer of exactly 32bits worth of data. */
  if ((ret = test_streaming (stream, buf, buf + (1 << 20), buf + (2 << 20), 1 << 12))) { goto fail; }

 fail:
  free (buf);
  return ret;
}

/***********************************************************************
 COMMAND LINE
 ***********************************************************************/

/* For each pair of command templates in the array below, test that
 * encoding and decoding commands work.  Also check for the expected
 * size delta, which should be approximately TEST_ADD_RATIO times the
 * file size created by test_make_inputs.  Due to differences in the
 * application header, it is suppressed (-A) so that all delta files
 * are the same. */
static int
test_command_line_arguments (xd3_stream *stream, int ignore)
{
  int i, ret;

  static const char* cmdpairs[] =
  {
    /* standard input, output */
    "%s %s -A < %s > %s", "%s -d < %s > %s",
    "%s %s -A -e < %s > %s", "%s -d < %s > %s",
    "%s %s -A= encode < %s > %s", "%s decode < %s > %s",
    "%s %s -A -q encode < %s > %s", "%s -qdq < %s > %s",

    /* file input, standard output */
    "%s %s -A= %s > %s", "%s -d %s > %s",
    "%s %s -A -e %s > %s", "%s -d %s > %s",
    "%s %s encode -A= %s > %s", "%s decode %s > %s",

    /* file input, output */
    "%s %s -A= %s %s", "%s -d %s %s",
    "%s %s -A -e %s %s", "%s -d %s %s",
    "%s %s -A= encode %s %s", "%s decode %s %s",

    /* option placement */
    "%s %s -A -f %s %s", "%s -f -d %s %s",
    "%s %s -e -A= %s %s", "%s -d -f %s %s",
    "%s %s -f encode -A= %s %s", "%s -f decode -f %s %s",
  };

  char ecmd[TESTBUFSIZE], dcmd[TESTBUFSIZE];
  int pairs = SIZEOF_ARRAY (cmdpairs) / 2;
  xoff_t tsize;
  xoff_t dsize;
  double ratio;

  mt_init (& static_mtrand, 0x9f73f7fc);

  for (i = 0; i < pairs; i += 1)
    {
      test_setup ();
      if ((ret = test_make_inputs (stream, NULL, & tsize))) { return ret; }

      sprintf (ecmd, cmdpairs[2*i], program_name,
	       test_softcfg_str, TEST_TARGET_FILE, TEST_DELTA_FILE);
      sprintf (dcmd, cmdpairs[2*i+1], program_name,
	       TEST_DELTA_FILE, TEST_RECON_FILE);

      /* Encode and decode. */
      if ((ret = system (ecmd)) != 0)
	{
	  DP(RINT "xdelta3: encode command: %s\n", ecmd);
	  stream->msg = "encode cmd failed";
	  return XD3_INTERNAL;
	}

      if ((ret = system (dcmd)) != 0)
	{
	  DP(RINT "xdelta3: decode command: %s\n", dcmd);
	  stream->msg = "decode cmd failed";
	  return XD3_INTERNAL;
	}

      /* Compare the target file. */
      if ((ret = compare_files (stream, TEST_TARGET_FILE, TEST_RECON_FILE)))
	{
	  return ret;
	}

      if ((ret = test_file_size (TEST_DELTA_FILE, & dsize)))
	{
	  return ret;
	}

      ratio = (double) dsize / (double) tsize;

      /* Check that it is not too small, not too large. */
      if (ratio >= TEST_ADD_RATIO + TEST_EPSILON)
	{
	  DP(RINT "xdelta3: test encode with size ratio %.4f, "
	     "expected < %.4f (%"Q"u, %"Q"u)\n",
	    ratio, TEST_ADD_RATIO + TEST_EPSILON, dsize, tsize);
	  stream->msg = "strange encoding";
	  return XD3_INTERNAL;
	}

      if (ratio <= TEST_ADD_RATIO * (1.0 - 2 * TEST_EPSILON))
	{
	  DP(RINT "xdelta3: test encode with size ratio %.4f, "
	     "expected > %.4f\n",
	    ratio, TEST_ADD_RATIO - TEST_EPSILON);
	  stream->msg = "strange encoding";
	  return XD3_INTERNAL;
	}

      /* Also check that compare_files works.  The delta and original should
       * not be identical. */
      if ((ret = compare_files (stream, TEST_DELTA_FILE,
				TEST_TARGET_FILE)) == 0)
	{
	  stream->msg = "broken compare_files";
	  return XD3_INTERNAL;
	}

      test_cleanup ();
      DOT ();
    }

  return 0;
}

static int
check_vcdiff_header (xd3_stream *stream,
		     const char *input,
		     const char *line_start,
		     const char *matches,
		     int yes_or_no)
{
  int ret;
  char vcmd[TESTBUFSIZE], gcmd[TESTBUFSIZE];

  sprintf (vcmd, "%s printhdr -f %s %s",
	   program_name, input, TEST_RECON2_FILE);

  if ((ret = system (vcmd)) != 0)
    {
      DP(RINT "xdelta3: printhdr command: %s\n", vcmd);
      stream->msg = "printhdr cmd failed";
      return XD3_INTERNAL;
    }

  sprintf (gcmd, "grep \"%s.*%s.*\" %s > /dev/null",
	   line_start, matches, TEST_RECON2_FILE);

  if (yes_or_no)
    {
      if ((ret = do_cmd (stream, gcmd)))
	{
	  DP(RINT "xdelta3: %s\n", gcmd);
	  return ret;
	}
    }
  else
    {
      if ((ret = do_fail (stream, gcmd)))
	{
	  DP(RINT "xdelta3: %s\n", gcmd);
	  return ret;
	}
    }

  return 0;
}

static int
test_recode_command2 (xd3_stream *stream, int has_source,
		      int variant, int change)
{
  int has_adler32 = (variant & 0x1) != 0;
  int has_apphead = (variant & 0x2) != 0;
  int has_secondary = (variant & 0x4) != 0;

  int change_adler32 = (change & 0x1) != 0;
  int change_apphead = (change & 0x2) != 0;
  int change_secondary = (change & 0x4) != 0;

  int recoded_adler32 = change_adler32 ? !has_adler32 : has_adler32;
  int recoded_apphead = change_apphead ? !has_apphead : has_apphead;
  int recoded_secondary = change_secondary ? !has_secondary : has_secondary;

  char ecmd[TESTBUFSIZE], recmd[TESTBUFSIZE], dcmd[TESTBUFSIZE];
  xoff_t tsize, ssize;
  int ret;

  test_setup ();

  if ((ret = test_make_inputs (stream, has_source ? & ssize : NULL, & tsize)))
    {
      return ret;
    }

  /* First encode */
  sprintf (ecmd, "%s %s -f ", program_name, test_softcfg_str);
  strcat (ecmd, has_adler32 ? "" : "-n ");
  strcat (ecmd, has_apphead ? "-A=encode_apphead " : "-A= ");
  strcat (ecmd, has_secondary ? "-S djw " : "-S none ");

  if (has_source)
    {
      strcat (ecmd, "-s ");
      strcat (ecmd, TEST_SOURCE_FILE);
      strcat (ecmd, " ");
    }

  strcat (ecmd, TEST_TARGET_FILE);
  strcat (ecmd, " ");
  strcat (ecmd, TEST_DELTA_FILE);

  if ((ret = system (ecmd)) != 0)
    {
      DP(RINT "xdelta3: encode command: %s\n", ecmd);
      stream->msg = "encode cmd failed";
      return XD3_INTERNAL;
    }

  /* Now recode */
  sprintf (recmd, "%s recode %s -f ", program_name, test_softcfg_str);
  strcat (recmd, recoded_adler32 ? "" : "-n ");
  strcat (recmd, !change_apphead ? "" :
	  (recoded_apphead ? "-A=recode_apphead " : "-A= "));
  strcat (recmd, recoded_secondary ? "-S djw " : "-S none ");
  strcat (recmd, TEST_DELTA_FILE);
  strcat (recmd, " ");
  strcat (recmd, TEST_COPY_FILE);

  if ((ret = system (recmd)) != 0)
    {
      DP(RINT "xdelta3: recode command: %s\n", recmd);
      stream->msg = "recode cmd failed";
      return XD3_INTERNAL;
    }

  /* Check recode changes. */
  if ((ret = check_vcdiff_header (stream,
				  TEST_COPY_FILE,
				  "VCDIFF window indicator",
				  "VCD_SOURCE",
				  has_source))) { return ret; }

  if ((ret = check_vcdiff_header (stream,
				  TEST_COPY_FILE,
				  "VCDIFF header indicator",
				  "VCD_SECONDARY",
				  recoded_secondary))) { return ret; }

  if ((ret = check_vcdiff_header (stream,
				  TEST_COPY_FILE,
				  "VCDIFF window indicator",
				  "VCD_ADLER32",
				  /* Recode can't generate an adler32
				   * checksum, it can only preserve it or
				   * remove it. */
				  has_adler32 && recoded_adler32)))
    {
      return ret;
    }

  if (!change_apphead)
    {
      if ((ret = check_vcdiff_header (stream,
				      TEST_COPY_FILE,
				      "VCDIFF header indicator",
				      "VCD_APPHEADER",
				      has_apphead)))
	{
	  return ret;
	}
      if ((ret = check_vcdiff_header (stream,
				      TEST_COPY_FILE,
				      "VCDIFF application header",
				      "encode_apphead",
				      has_apphead)))
	{
	  return ret;
	}
    }
  else
    {
      if ((ret = check_vcdiff_header (stream,
				      TEST_COPY_FILE,
				      "VCDIFF header indicator",
				      "VCD_APPHEADER",
				      recoded_apphead)))
	{
	  return ret;
	}
      if (recoded_apphead &&
	  (ret = check_vcdiff_header (stream,
				      TEST_COPY_FILE,
				      "VCDIFF application header",
				      "recode_apphead",
				      1)))
	{
	  return ret;
	}
    }

  /* Now decode */
  sprintf (dcmd, "%s -fd ", program_name);
  
  if (has_source)
    {
      strcat (dcmd, "-s ");
      strcat (dcmd, TEST_SOURCE_FILE);
      strcat (dcmd, " ");
    }

  strcat (dcmd, TEST_COPY_FILE);
  strcat (dcmd, " ");
  strcat (dcmd, TEST_RECON_FILE);

  if ((ret = system (dcmd)) != 0)
    {
      DP(RINT "xdelta3: decode command: %s\n", dcmd);
      stream->msg = "decode cmd failed";
      return XD3_INTERNAL;
    }

  /* Now compare. */
  if ((ret = compare_files (stream, TEST_TARGET_FILE, TEST_RECON_FILE)))
    {
      return ret;
    }

  return 0;
}  

static int
test_recode_command (xd3_stream *stream, int ignore)
{
  /* Things to test:
   * - with and without a source file (recode does not change)
   * 
   * (recode may or may not change -- 8 variations)
   * - with and without adler32
   * - with and without app header
   * - with and without secondary
   */
  int has_source;
  int variant;
  int change;
  int ret;

  for (has_source = 0; has_source < 2; has_source++)
    {
      for (variant = 0; variant < 8; variant++)
	{
	  for (change = 0; change < 8; change++)
	    {
	      if ((ret = test_recode_command2 (stream, has_source,
					       variant, change)))
		{
		  return ret;
		}
	    }
	  DOT ();
	}
    }

  return 0;
}

/***********************************************************************
 EXTERNAL I/O DECOMPRESSION/RECOMPRESSION
 ***********************************************************************/

#if EXTERNAL_COMPRESSION
/* This performs one step of the test_externally_compressed_io
 * function described below.  It builds a pipe containing both Xdelta
 * and external compression/decompression that should not modify the
 * data passing through. */
static int
test_compressed_pipe (xd3_stream *stream, main_extcomp *ext, char* buf,
		      const char* comp_options, const char* decomp_options,
		      int do_ext_recomp, const char* msg)
{
  int ret;
  char decomp_buf[TESTBUFSIZE];

  if (do_ext_recomp)
    {
      sprintf (decomp_buf, " | %s %s", ext->decomp_cmdname, ext->decomp_options);
    }
  else
    {
      decomp_buf[0] = 0;
    }

  sprintf (buf, "%s %s < %s | %s %s | %s %s%s > %s",
	   ext->recomp_cmdname, ext->recomp_options,
	   TEST_TARGET_FILE,
	   program_name, comp_options,
	   program_name, decomp_options,
	   decomp_buf,
	   TEST_RECON_FILE);

  if ((ret = system (buf)) != 0)
    {
      stream->msg = msg;
      return XD3_INTERNAL;
    }

  if ((ret = compare_files (stream, TEST_TARGET_FILE, TEST_RECON_FILE)))
    {
      return XD3_INTERNAL;
    }

  DOT ();
  return 0;
}

/* We want to test that a pipe such as:
 *
 * --> | gzip -cf | xdelta3 -cf | xdelta3 -dcf | gzip -dcf | -->
 *
 * is transparent, i.e., does not modify the stream of data.  However,
 * we also want to verify that at the center the data is properly
 * compressed, i.e., that we do not just have a re-compressed gzip
 * format, that we have an VCDIFF format.  We do this in two steps.
 * First test the above pipe, then test with suppressed output
 * recompression (-D).  The result should be the original input:
 *
 * --> | gzip -cf | xdelta3 -cf | xdelta3 -Ddcf | -->
 *
 * Finally we want to test that -D also disables input decompression:
 *
 * --> | gzip -cf | xdelta3 -Dcf | xdelta3 -Ddcf | gzip -dcf | -->
 */
static int
test_externally_compressed_io (xd3_stream *stream, int ignore)
{
  usize_t i;
  int ret;
  char buf[TESTBUFSIZE];

  mt_init (& static_mtrand, 0x9f73f7fc);

  if ((ret = test_make_inputs (stream, NULL, NULL))) { return ret; }

  for (i = 0; i < SIZEOF_ARRAY (extcomp_types); i += 1)
    {
      main_extcomp *ext = & extcomp_types[i];

      /* Test for the existence of the external command first, if not skip. */
      sprintf (buf, "%s %s < /dev/null > /dev/null", ext->recomp_cmdname, ext->recomp_options);

      if ((ret = system (buf)) != 0)
	{
	  DP(RINT "%s=0", ext->recomp_cmdname);
	  continue;
	}

      if ((ret = test_compressed_pipe (stream, ext, buf, "-cfq", "-dcfq", 1,
				       "compression failed: identity pipe")) ||
	  (ret = test_compressed_pipe (stream, ext, buf, "-cfq", "-Rdcfq", 0,
				       "compression failed: without recompression")) ||
	  (ret = test_compressed_pipe (stream, ext, buf, "-Dcfq", "-Rdcfq", 1,
				       "compression failed: without decompression")))
	{
	  return ret;
	}
    }

  return 0;
}

/* This tests the proper functioning of external decompression for
 * source files.  The source and target files are identical and
 * compressed by gzip.  Decoding such a delta with recompression
 * disbaled (-R) should produce the original, uncompressed
 * source/target file.  Then it checks with output recompression
 * enabled--in this case the output should be a compressed copy of the
 * original source/target file.  Then it checks that encoding with
 * decompression disabled works--the compressed files are identical
 * and decoding them should always produce a compressed output,
 * regardless of -R since the encoded delta file had decompression
 * disabled..
 */
static int
test_source_decompression (xd3_stream *stream, int ignore)
{
  int ret;
  char buf[TESTBUFSIZE];
  const main_extcomp *ext;

  mt_init (& static_mtrand, 0x9f73f7fc);

  test_setup ();
  if ((ret = test_make_inputs (stream, NULL, NULL))) { return ret; }

  /* Use gzip. */
  if ((ext = main_get_compressor ("G")) == NULL) { DP(RINT "skipped"); return 0; }

  /* Save an uncompressed copy. */
  if ((ret = test_save_copy (TEST_TARGET_FILE))) { return ret; }

  /* Compress the target. */
  sprintf (buf, "%s %s < %s > %s", ext->recomp_cmdname,
	   ext->recomp_options, TEST_TARGET_FILE, TEST_SOURCE_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  /* Copy back to the source. */
  sprintf (buf, "cp -f %s %s", TEST_SOURCE_FILE, TEST_TARGET_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  /* Now the two identical files are compressed.  Delta-encode the target,
   * with decompression. */
  sprintf (buf, "%s -eq -s%s %s %s", program_name, TEST_SOURCE_FILE, TEST_TARGET_FILE, TEST_DELTA_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  /* Decode the delta file with recompression disabled, should get an
   * uncompressed file out. */
  sprintf (buf, "%s -dq -R -s%s %s %s", program_name, TEST_SOURCE_FILE, TEST_DELTA_FILE, TEST_RECON_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }
  if ((ret = compare_files (stream, TEST_COPY_FILE, TEST_RECON_FILE))) { return ret; }

  /* Decode the delta file with recompression, should get a compressed file
   * out.  But we can't compare compressed files directly. */
  sprintf (buf, "%s -dqf -s%s %s %s", program_name, TEST_SOURCE_FILE, TEST_DELTA_FILE, TEST_RECON_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }
  sprintf (buf, "%s %s < %s > %s", ext->decomp_cmdname, ext->decomp_options, TEST_RECON_FILE, TEST_RECON2_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }
  if ((ret = compare_files (stream, TEST_COPY_FILE, TEST_RECON2_FILE))) { return ret; }

  /* Encode with decompression disabled */
  sprintf (buf, "%s -feqD -s%s %s %s", program_name, TEST_SOURCE_FILE, TEST_TARGET_FILE, TEST_DELTA_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  /* Decode the delta file with recompression enabled, it doesn't matter,
   * should get the compressed file out. */
  sprintf (buf, "%s -fdq -s%s %s %s", program_name, TEST_SOURCE_FILE, TEST_DELTA_FILE, TEST_RECON_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }
  if ((ret = compare_files (stream, TEST_TARGET_FILE, TEST_RECON_FILE))) { return ret; }

  /* Try again with recompression disabled, it doesn't make a difference. */
  sprintf (buf, "%s -fqRd -s%s %s %s", program_name, TEST_SOURCE_FILE, TEST_DELTA_FILE, TEST_RECON_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }
  if ((ret = compare_files (stream, TEST_TARGET_FILE, TEST_RECON_FILE))) { return ret; }
  test_cleanup();
  return 0;
}
#endif

/***********************************************************************
 FORCE, STDOUT
 ***********************************************************************/

/* This tests that output will not overwrite an existing file unless
 * -f was specified.  The test is for encoding (the same code handles
 * it for decoding). */
static int
test_force_behavior (xd3_stream *stream, int ignore)
{
  int ret;
  char buf[TESTBUFSIZE];

  /* Create empty target file */
  test_setup ();
  sprintf (buf, "cp /dev/null %s", TEST_TARGET_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  /* Encode to delta file */
  sprintf (buf, "%s -e %s %s", program_name,
	   TEST_TARGET_FILE, TEST_DELTA_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  /* Encode again, should fail. */
  sprintf (buf, "%s -q -e %s %s ", program_name,
	   TEST_TARGET_FILE, TEST_DELTA_FILE);
  if ((ret = do_fail (stream, buf))) { return ret; }

  /* Force it, should succeed. */
  sprintf (buf, "%s -f -e %s %s", program_name,
	   TEST_TARGET_FILE, TEST_DELTA_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }
  test_cleanup();
  return 0;
}

/* This checks the proper operation of the -c flag.  When specified
 * the default output becomes stdout, otherwise the input must be
 * provided (encode) or it may be defaulted (decode w/ app header). */
static int
test_stdout_behavior (xd3_stream *stream, int ignore)
{
  int ret;
  char buf[TESTBUFSIZE];

  test_setup();
  sprintf (buf, "cp /dev/null %s", TEST_TARGET_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  /* Without -c, encode writes to delta file */
  sprintf (buf, "%s -e %s %s", program_name,
	   TEST_TARGET_FILE, TEST_DELTA_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  /* With -c, encode writes to stdout */
  sprintf (buf, "%s -e -c %s > %s", program_name,
	   TEST_TARGET_FILE, TEST_DELTA_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  /* Without -c, decode writes to target file name, but it fails because the
   * file exists. */
  sprintf (buf, "%s -q -d %s ", program_name, TEST_DELTA_FILE);
  if ((ret = do_fail (stream, buf))) { return ret; }

  /* With -c, decode writes to stdout */
  sprintf (buf, "%s -d -c %s > /dev/null", program_name, TEST_DELTA_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }
  test_cleanup();

  return 0;
}

/* This tests that the no-output flag (-J) works. */
static int
test_no_output (xd3_stream *stream, int ignore)
{
  int ret;
  char buf[TESTBUFSIZE];

  test_setup ();

  sprintf (buf, "touch %s && chmod 0000 %s",
	   TEST_NOPERM_FILE, TEST_NOPERM_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  if ((ret = test_make_inputs (stream, NULL, NULL))) { return ret; }

  /* Try no_output encode w/out unwritable output file */
  sprintf (buf, "%s -q -f -e %s %s", program_name,
	   TEST_TARGET_FILE, TEST_NOPERM_FILE);
  if ((ret = do_fail (stream, buf))) { return ret; }
  sprintf (buf, "%s -J -e %s %s", program_name,
	   TEST_TARGET_FILE, TEST_NOPERM_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  /* Now really write the delta to test decode no-output */
  sprintf (buf, "%s -e %s %s", program_name,
	   TEST_TARGET_FILE, TEST_DELTA_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }

  sprintf (buf, "%s -q -f -d %s %s", program_name,
	   TEST_DELTA_FILE, TEST_NOPERM_FILE);
  if ((ret = do_fail (stream, buf))) { return ret; }
  sprintf (buf, "%s -J -d %s %s", program_name,
	   TEST_DELTA_FILE, TEST_NOPERM_FILE);
  if ((ret = do_cmd (stream, buf))) { return ret; }
  test_cleanup ();
  return 0;
}

/***********************************************************************
 Source identical optimization
 ***********************************************************************/

/* Computing a delta should be fastest when the two inputs are
 * identical, this checks it.  The library is called to compute a
 * delta between a 10000 byte file, 1000 byte winsize, 500 byte source
 * blocksize.  The same buffer is used for both source and target. */
static int
test_identical_behavior (xd3_stream *stream, int ignore)
{
#define IDB_TGTSZ 10000
#define IDB_BLKSZ 500
#define IDB_WINSZ 1000
#define IDB_DELSZ 1000
#define IDB_WINCNT (IDB_TGTSZ / IDB_WINSZ)

  int ret, i;
  uint8_t buf[IDB_TGTSZ];
  uint8_t del[IDB_DELSZ];
  uint8_t rec[IDB_TGTSZ];
  xd3_source source;
  int nextencwin = 0;
  int winstarts = 0, winfinishes = 0;
  xoff_t srcwin = -1;
  usize_t delpos = 0, recsize;
  xd3_config config;

  for (i = 0; i < IDB_TGTSZ; i += 1) { buf[i] = mt_random (&static_mtrand); }

  stream->winsize = IDB_WINSZ;

  source.size     = IDB_TGTSZ;
  source.blksize  = IDB_BLKSZ;
  source.name     = "";
  source.curblk   = NULL;
  source.curblkno = -1;

  if ((ret = xd3_set_source (stream, & source))) { goto fail; }

  /* Compute an delta between identical source and targets. */
  for (;;)
    {
      ret = xd3_encode_input (stream);

      if (ret == XD3_INPUT)
	{
	  xd3_avail_input (stream, buf + (IDB_WINSZ * nextencwin), IDB_WINSZ);
	  nextencwin += 1;
	  continue;
	}

      if (ret == XD3_GETSRCBLK)
	{
	  source.curblkno = source.getblkno;
	  source.onblk    = IDB_BLKSZ;
	  source.curblk   = buf + source.getblkno * IDB_BLKSZ;
	  srcwin = source.getblkno;
	  continue;
	}

      if (ret == XD3_WINSTART)
	{
	  winstarts++;
	  continue;
	}
      if (ret == XD3_WINFINISH)
	{
	  winfinishes++;
	  if (winfinishes == IDB_WINCNT)
	    {
	      break;
	    }
	  continue;
	}

      if (ret != XD3_OUTPUT) { goto fail; }

      CHECK(delpos + stream->avail_out <= IDB_DELSZ);

      memcpy (del + delpos, stream->next_out, stream->avail_out);

      delpos += stream->avail_out;

      xd3_consume_output (stream);
    }

  CHECK(srcwin == source.blocks - 1);
  CHECK(winfinishes == IDB_WINCNT);
  CHECK(winstarts == IDB_WINCNT);
  CHECK(nextencwin == IDB_WINCNT);

  /* Reset. */
  source.blksize  = IDB_TGTSZ;
  source.onblk    = IDB_TGTSZ;
  source.curblk   = buf;
  source.curblkno = 0;

  if ((ret = xd3_close_stream (stream))) { goto fail; }
  xd3_free_stream (stream);
  xd3_init_config (& config, 0);
  if ((ret = xd3_config_stream (stream, & config))) { goto fail; }
  if ((ret = xd3_set_source (stream, & source))) { goto fail; }

  /* Decode. */
  if ((ret = xd3_decode_stream (stream, del, delpos, rec, & recsize, IDB_TGTSZ))) { goto fail; }

  /* Check result size and data. */
  if (recsize != IDB_TGTSZ) { stream->msg = "wrong size reconstruction"; goto fail; }
  if (memcmp (rec, buf, IDB_TGTSZ) != 0) { stream->msg = "wrong data reconstruction"; goto fail; }

  /* Check that there was one copy per window. */
  IF_DEBUG (if (stream->n_scpy != IDB_WINCNT ||
		stream->n_add != 0 ||
		stream->n_run != 0) { stream->msg = "wrong copy count"; goto fail; });

  /* Check that no checksums were computed because the initial match
     was presumed. */
  IF_DEBUG (if (stream->large_ckcnt != 0) { stream->msg = "wrong checksum behavior"; goto fail; });

  ret = 0;
 fail:
  return ret;
}

/***********************************************************************
 String matching test
 ***********************************************************************/

/* Check particular matching behaviors by calling
 * xd3_string_match_soft directly with specific arguments. */
typedef struct _string_match_test string_match_test;

typedef enum
{
  SM_NONE    = 0,
  SM_LAZY    = (1 << 1),
} string_match_flags;

struct _string_match_test
{
  const char *input;
  int         flags;
  const char *result;
};

static const string_match_test match_tests[] =
{
  /* nothing */
  { "1234567890", SM_NONE, "" },

  /* basic run, copy */
  { "11111111112323232323", SM_NONE, "R0/10 C12/8@10" },

  /* no run smaller than MIN_RUN=8 */
  { "1111111",  SM_NONE, "C1/6@0" },
  { "11111111", SM_NONE, "R0/8" },

  /* simple promotion: the third copy address depends on promotion */
  { "ABCDEF_ABCDEF^ABCDEF", SM_NONE,    "C7/6@0 C14/6@7" },
  /* { "ABCDEF_ABCDEF^ABCDEF", SM_PROMOTE, "C7/6@0 C14/6@0" }, forgotten */

  /* simple lazy: there is a better copy starting with "23 X" than "123 " */
  { "123 23 XYZ 123 XYZ", SM_NONE, "C11/4@0" },
  { "123 23 XYZ 123 XYZ", SM_LAZY, "C11/4@0 C12/6@4" },

  /* trylazy: no lazy matches unless there are at least two characters beyond
   * the first match */
  { "2123_121212",   SM_LAZY, "C7/4@5" },
  { "2123_1212123",  SM_LAZY, "C7/4@5" },
  { "2123_1212123_", SM_LAZY, "C7/4@5 C8/5@0" },

  /* trylazy: no lazy matches if the copy is >= MAXLAZY=10 */
  { "2123_121212123_",   SM_LAZY, "C7/6@5 C10/5@0" },
  { "2123_12121212123_", SM_LAZY, "C7/8@5 C12/5@0" },
  { "2123_1212121212123_", SM_LAZY, "C7/10@5" },

  /* lazy run: check a run overlapped by a longer copy */
  { "11111112 111111112 1", SM_LAZY, "C1/6@0 R9/8 C10/10@0" },

  /* lazy match: match_length,run_l >= min_match tests, shouldn't get any
   * copies within the run, no run within the copy */
  { "^________^________  ", SM_LAZY, "R1/8 C9/9@0" },

  /* chain depth: it only goes back 10. this checks that the 10th match hits
   * and the 11th misses. */
  { "1234 1234_1234-1234=1234+1234[1234]1234{1234}1234<1234 ", SM_NONE,
    "C5/4@0 C10/4@5 C15/4@10 C20/4@15 C25/4@20 C30/4@25 C35/4@30 C40/4@35 C45/4@40 C50/5@0" },
  { "1234 1234_1234-1234=1234+1234[1234]1234{1234}1234<1234>1234 ", SM_NONE,
    "C5/4@0 C10/4@5 C15/4@10 C20/4@15 C25/4@20 C30/4@25 C35/4@30 C40/4@35 C45/4@40 C50/4@45 C55/4@50" },

  /* ssmatch test */
  { "ABCDE___ABCDE*** BCDE***", SM_NONE, "C8/5@0 C17/4@1" },
  /*{ "ABCDE___ABCDE*** BCDE***", SM_SSMATCH, "C8/5@0 C17/7@9" }, forgotten */
};

static int
test_string_matching (xd3_stream *stream, int ignore)
{
  usize_t i;
  int ret;
  xd3_config config;
  char rbuf[TESTBUFSIZE];

  for (i = 0; i < SIZEOF_ARRAY (match_tests); i += 1)
    {
      const string_match_test *test = & match_tests[i];
      char *rptr = rbuf;
      usize_t len = strlen (test->input);

      xd3_free_stream (stream);
      xd3_init_config (& config, 0);

      config.smatch_cfg   = XD3_SMATCH_SOFT;
      config.smatcher_soft.large_look   = 4;
      config.smatcher_soft.large_step   = 4;
      config.smatcher_soft.small_look   = 4;
      config.smatcher_soft.small_chain  = 10;
      config.smatcher_soft.small_lchain = 10;
      config.smatcher_soft.max_lazy     = (test->flags & SM_LAZY) ? 10 : 0;
      config.smatcher_soft.long_enough  = 10;

      if ((ret = xd3_config_stream (stream, & config))) { return ret; }
      if ((ret = xd3_encode_init_full (stream))) { return ret; }

      xd3_avail_input (stream, (uint8_t*)test->input, len);

      if ((ret = stream->smatcher.string_match (stream))) { return ret; }

      *rptr = 0;
      while (! xd3_rlist_empty (& stream->iopt_used))
	{
	  xd3_rinst *inst = xd3_rlist_pop_front (& stream->iopt_used);

	  switch (inst->type)
	    {
	    case XD3_RUN: *rptr++ = 'R'; break;
	    case XD3_CPY: *rptr++ = 'C'; break;
	    default: CHECK(0);
	    }

	  sprintf (rptr, "%d/%d", inst->pos, inst->size);
	  rptr += strlen (rptr);

	  if (inst->type == XD3_CPY)
	    {
	      *rptr++ = '@';
	      sprintf (rptr, "%"Q"d", inst->addr);
	      rptr += strlen (rptr);
	    }

	  *rptr++ = ' ';

	  xd3_rlist_push_back (& stream->iopt_free, inst);
	}

      if (rptr != rbuf)
	{
	  rptr -= 1; *rptr = 0;
	}

      if (strcmp (rbuf, test->result) != 0)
	{
	  DP(RINT "test %u: expected %s: got %s", i, test->result, rbuf);
	  stream->msg = "wrong result";
	  return XD3_INTERNAL;
	}
    }

  return 0;
}

/*
 * This is a test for many overlapping instructions. It must be a lazy
 * matcher.
 */
static int
test_iopt_flush_instructions (xd3_stream *stream, int ignore)
{
  int ret, i;
  usize_t tpos = 0;
  usize_t delta_size, recon_size;
  xd3_config config;
  uint8_t target[TESTBUFSIZE];
  uint8_t delta[TESTBUFSIZE];
  uint8_t recon[TESTBUFSIZE];

  xd3_free_stream (stream);
  xd3_init_config (& config, 0);

  config.smatch_cfg    = XD3_SMATCH_SOFT;
  config.smatcher_soft.large_look    = 16;
  config.smatcher_soft.large_step    = 16;
  config.smatcher_soft.small_look    = 4;
  config.smatcher_soft.small_chain   = 128;
  config.smatcher_soft.small_lchain  = 16;
  config.smatcher_soft.max_lazy      = 8;
  config.smatcher_soft.long_enough   = 128;

  if ((ret = xd3_config_stream (stream, & config))) { return ret; }

  for (i = 1; i < 250; i++)
    {
      target[tpos++] = i;
      target[tpos++] = i+1;
      target[tpos++] = i+2;
      target[tpos++] = i+3;
      target[tpos++] = 0;
    }
  for (i = 1; i < 253; i++)
    {
      target[tpos++] = i;
    }

  if ((ret = xd3_encode_stream (stream, target, tpos,
				    delta, & delta_size, sizeof (delta))))
    {
      return ret;
    }

  xd3_free_stream(stream);
  if ((ret = xd3_config_stream (stream, & config))) { return ret; }

  if ((ret = xd3_decode_stream (stream, delta, delta_size,
				recon, & recon_size, sizeof (recon))))
    {
      return ret;
    }

  CHECK(tpos == recon_size);
  CHECK(memcmp(target, recon, recon_size) == 0);

  return 0;
}

/*
 * This tests the 32/64bit ambiguity for source-window matching.
 */
static int
test_source_cksum_offset (xd3_stream *stream, int ignore)
{
  xd3_source source;

  // Inputs are:
  struct {
    xoff_t   cpos;   // stream->srcwin_cksum_pos;
    xoff_t   ipos;   // stream->total_in;
    xoff_t   size;   // stream->src->size;

    usize_t  input;  // input  32-bit offset
    xoff_t   output; // output 64-bit offset

  } cksum_test[] = {
    // If cpos is <= 2^32
    { 1, 1, 1, 1, 1 },

#if XD3_USE_LARGEFILE64
//    cpos            ipos            size            input         output
//    0x____xxxxxULL, 0x____xxxxxULL, 0x____xxxxxULL, 0x___xxxxxUL, 0x____xxxxxULL
    { 0x100100000ULL, 0x100000000ULL, 0x100200000ULL, 0x00000000UL, 0x100000000ULL },
    { 0x100100000ULL, 0x100000000ULL, 0x100200000ULL, 0xF0000000UL, 0x0F0000000ULL },

    { 0x100200000ULL, 0x100100000ULL, 0x100200000ULL, 0x00300000UL, 0x000300000ULL },

    { 25771983104ULL, 25770000000ULL, 26414808769ULL, 2139216707UL, 23614053187ULL },

#endif

    { 0, 0, 0, 0, 0 },
  }, *test_ptr;

  stream->src = &source;

  for (test_ptr = cksum_test; test_ptr->cpos; test_ptr++) {
	xoff_t r;
    stream->srcwin_cksum_pos = test_ptr->cpos;
    stream->total_in = test_ptr->ipos;
    stream->src->size = test_ptr->size;

    r = xd3_source_cksum_offset(stream, test_ptr->input);
    CHECK(r == test_ptr->output);
  }
  return 0;
}

static int
test_in_memory (xd3_stream *stream, int ignore)
{
  // test_text is 256 bytes
  uint8_t ibuf[sizeof(test_text)];
  uint8_t dbuf[sizeof(test_text)];
  uint8_t obuf[sizeof(test_text)];
  usize_t size = sizeof(test_text);
  usize_t dsize, osize;
  int r1, r2;
  int eflags = SECONDARY_DJW ? XD3_SEC_DJW : 0;

  memcpy(ibuf, test_text, size);
  memset(ibuf + 128, 0, 16);

  r1 = xd3_encode_memory(ibuf, size,
			 test_text, size,
			 dbuf, &dsize, size, eflags);

  r2 = xd3_decode_memory(dbuf, dsize,
			 test_text, size,
			 obuf, &osize, size, 0);

  if (r1 != 0 || r2 != 0 || dsize >= (size/2) || dsize < 1 ||
      osize != size) {
    stream->msg = "encode/decode size error";
    return XD3_INTERNAL;
  }

  if (memcmp(obuf, ibuf, size) != 0) {
    stream->msg = "encode/decode data error";
    return XD3_INTERNAL;
  }

  return 0;
}

/***********************************************************************
 TEST MAIN
 ***********************************************************************/

static int
xd3_selftest (void)
{
#define DO_TEST(fn,flags,arg)                                         \
  do {                                                                \
    xd3_stream stream;                                                \
    xd3_config config;                                                \
    xd3_init_config (& config, flags);                                \
    DP(RINT "xdelta3: testing " #fn "%s...",                          \
             flags ? (" (" #flags ")") : "");                         \
    if ((ret = xd3_config_stream (& stream, & config) == 0) &&        \
        (ret = test_ ## fn (& stream, arg)) == 0) {                   \
      DP(RINT " success\n");                                          \
    } else {                                                          \
      DP(RINT " failed: %s: %s\n", xd3_errstring (& stream),          \
               xd3_mainerror (ret)); }                                \
    xd3_free_stream (& stream);                                       \
    if (ret != 0) { goto failure; }                                   \
  } while (0)

  int ret;

#ifndef WIN32
  if (getuid() == 0)
    {
      DP(RINT "xdelta3: This test should not be run as root.\n");
      ret = XD3_INVALID;
      goto failure;
    }
#endif

  DO_TEST (random_numbers, 0, 0);

  DO_TEST (decode_integer_end_of_input, 0, 0);
  DO_TEST (decode_integer_overflow, 0, 0);
  DO_TEST (encode_decode_uint32_t, 0, 0);
  DO_TEST (encode_decode_uint64_t, 0, 0);
  DO_TEST (usize_t_overflow, 0, 0);
  DO_TEST (forward_match, 0, 0);

  DO_TEST (address_cache, 0, 0);
  IF_GENCODETBL (DO_TEST (address_cache, XD3_ALT_CODE_TABLE, 0));

  DO_TEST (string_matching, 0, 0);
  DO_TEST (choose_instruction, 0, 0);
  DO_TEST (identical_behavior, 0, 0);
  DO_TEST (in_memory, 0, 0);

  IF_GENCODETBL (DO_TEST (choose_instruction, XD3_ALT_CODE_TABLE, 0));
  IF_GENCODETBL (DO_TEST (encode_code_table, 0, 0));

  DO_TEST (iopt_flush_instructions, 0, 0);
  DO_TEST (source_cksum_offset, 0, 0);

  DO_TEST (decompress_single_bit_error, 0, 3);
  DO_TEST (decompress_single_bit_error, XD3_ADLER32, 3);

  IF_FGK (DO_TEST (decompress_single_bit_error, XD3_SEC_FGK, 3));
  IF_DJW (DO_TEST (decompress_single_bit_error, XD3_SEC_DJW, 8));

  /* There are many expected non-failures for ALT_CODE_TABLE because
   * not all of the instruction codes are used. */
  IF_GENCODETBL (
	 DO_TEST (decompress_single_bit_error, XD3_ALT_CODE_TABLE, 224));

#ifndef WIN32
  DO_TEST (force_behavior, 0, 0);
  DO_TEST (stdout_behavior, 0, 0);
  DO_TEST (no_output, 0, 0);
  DO_TEST (command_line_arguments, 0, 0);
  DO_TEST (recode_command, 0, 0);

#if EXTERNAL_COMPRESSION
  DO_TEST (source_decompression, 0, 0);
  DO_TEST (externally_compressed_io, 0, 0);
#endif

#endif /* WIN32 */

  IF_DJW (DO_TEST (secondary_huff, 0, DJW_MAX_GROUPS));
  IF_FGK (DO_TEST (secondary_fgk, 0, 1));

  DO_TEST (compressed_stream_overflow, 0, 0);

failure:
  test_cleanup ();
  return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
#undef DO_TEST
}
