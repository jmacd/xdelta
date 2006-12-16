#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define BUFSZ (1 << 22)

typedef unsigned int usize_t;

double error_prob   = 0.0001;
usize_t mean_change  = 100;
usize_t total_change = 0;
usize_t total_size   = 0;
usize_t max_change   = 0;
usize_t num_change   = 0;

int last_end = 0;

#ifdef WIN32
// whatever
static 
double drand48() {
  double r = rand() / (double)RAND_MAX;
  return r;
}
long lrand48() {
	long l = 0;
	int i;
	for (i = 0; i < 32; i++) {
		l = l ^ (l << 2) ^ (l << 1) ^ rand();
	}
	return l;
}
#endif

static usize_t
edist (usize_t mean, usize_t max)
{
  double mean_d = mean;
  double erand  = log (1.0 / drand48 ());
  usize_t x = (usize_t) (mean_d * erand + 0.5);

  return (x < max) ? (x > 0 ? x : 1) : max;
}

void modify (char *buf, usize_t size)
{
  usize_t bufpos = 0, j;

  last_end = 0;

  for (;; /* bufpos and j are incremented in the inner loop */)
    {
      /* The size of the next modification. */
      usize_t next_size = edist (mean_change, 1 << 31);
      /* The expected interval of such a change. */
      double expect_interval = ((double) next_size * (1.0 - error_prob)) / error_prob;
      /* The number of bytes until the next modification. */
      usize_t next_mod  = edist ((usize_t)expect_interval, 1 << 31);

      if (next_size + next_mod + bufpos > size) { break; }

      if (max_change < next_size) { max_change = next_size; }

      bufpos += next_mod;

      fprintf (stderr, "COPY: %u-%u (%u)\n", total_size + last_end, total_size + bufpos, bufpos - last_end);
      fprintf (stderr, "ADD:  %u-%u (%u) is change %u\n", total_size + bufpos , total_size + bufpos + next_size, next_size, num_change);

      total_change += next_size;
      num_change   += 1;

      for (j = 0; j < next_size; j += 1, bufpos += 1)
	{
	  buf[bufpos] = (char)(lrand48 () >> 3);
	}

      last_end = bufpos;
    }

  fprintf (stderr, "COPY: %u-%u (%u)\n", total_size + last_end, total_size + size, size - last_end);

  total_size += size;
}

int main(int argc, char **argv)
{
  FILE *in, *out;
  char *buf = malloc(BUFSZ);
  int c, ret;

  if (argc > 5)
    {
      fprintf (stderr, "usage: badcopy [byte_error_prob [mean_error_size]]\n");
      return 1;
    }

  if (argc > 4) { mean_change = atoi (argv[4]); }
  if (argc > 3) { error_prob  = atof (argv[3]); }
  fprintf (stderr, "mean change = %u; error_prob = %0.10f\n", mean_change, error_prob);

  in = fopen(argv[1], "rb");
  out = fopen(argv[2], "wb");

  if (in == NULL) {
	  fprintf (stderr, "Cannot open input: %s\n", argv[1]);
	  return 1;
  }
  if (out == NULL) {
	  fprintf (stderr, "Cannot open output: %s\n", argv[2]);
	  return 1;
  }

  if (error_prob < 0.0 || error_prob > 1.0)
    {
      fprintf (stderr, "warning: error probability out of range\n");
      return 1;
    }

  do
    {
      c = fread (buf, 1, BUFSZ, in);

      if (c == 0) { break; }

      modify (buf, c);

      ret = fwrite (buf, 1, c, out);
    }
  while (c == BUFSZ);

  if ((ret = fclose (stdout)))
    {
      perror ("fclose");
      return 1;
    }

  fprintf (stderr, "add_prob %f; %u adds; total_change %u of %u bytes; add percentage %f; max add size %u\n",
	   error_prob, num_change, total_change, total_size, (double) total_change / (double) total_size, max_change);

  return 0;
}
