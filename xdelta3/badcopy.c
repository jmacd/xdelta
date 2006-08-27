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

static int
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
      usize_t next_mod  = edist (expect_interval, 1 << 31);

      if (next_size + next_mod + bufpos > size) { break; }

      if (max_change < next_size) { max_change = next_size; }

      bufpos += next_mod;

      fprintf (stderr, "COPY: %u-%u (%u)\n", total_size + last_end, total_size + bufpos, bufpos - last_end);

      fprintf (stderr, "ADD:  %u-%u (%u) is change %u\n", total_size + bufpos , total_size + bufpos + next_size, next_size, num_change);

      total_change += next_size;
      num_change   += 1;

      for (j = 0; j < next_size; j += 1, bufpos += 1)
	{
	  buf[bufpos] = lrand48 () >> 3;
	}

      last_end = bufpos;
    }

  fprintf (stderr, "COPY: %u-%u (%u)\n", total_size + last_end, total_size + size, size - last_end);

  total_size += size;
}

int main(int argc, char **argv)
{
  char buf[BUFSZ];
  int c, ret;

  if (argc > 3)
    {
      fprintf (stderr, "usage: badcopy [byte_error_prob [mean_error_size]]\n");
      return 1;
    }

  if (argc > 2) { mean_change = atoi (argv[2]); }
  if (argc > 1) { error_prob  = atof (argv[1]); }

  if (error_prob < 0.0 || error_prob > 1.0)
    {
      fprintf (stderr, "warning: error probability out of range\n");
      return 1;
    }

  do
    {
      c = fread (buf, 1, BUFSZ, stdin);

      if (c == 0) { break; }

      modify (buf, c);

      ret = fwrite (buf, 1, c, stdout);
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
