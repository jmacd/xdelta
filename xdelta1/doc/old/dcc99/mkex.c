#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <ctype.h>

#define LEN 10
#define COUNT 7
#define THRESH 5

char example[LEN+1][COUNT];
int example_no;

char* words[1<<20];
int word_no;

int frag_count = 2;

int main()
{
  int i, j, k;
  char * buf = g_new0 (char,2048);

  example_no = 1;
  g_assert (strlen ("difference") == LEN);
  memcpy (example[0], "difference", LEN);

  while (fgets (buf, 2048, stdin))
    {
      if (strlen(buf) < (LEN+1))
	continue;

      buf[LEN] = 0;

      for (i = 0; i < LEN; i += 1)
	buf[i] = tolower(buf[i]);

      words[word_no++] = g_strdup (buf);
    }

  for (i = 0; i < word_no; i += 1)
    {
      for (j = 0; j < word_no; j += 1)
	{
	  char* w1 = words[i];
	  char* w2 = words[j];
	  int in_match = TRUE, m = 0, f = 0;

	  if (i == j)
	    continue;

	  if (w1[0] == w2[0])
	    continue;

	  for (k = 0; k < LEN; k += 1)
	    {
	      if (w1[k] == w2[k])
		{
		  if (! in_match)
		    f += 1;

		  m += 1;
		  in_match = TRUE;
		}
	      else
		in_match = FALSE;
	    }

	  if (f == frag_count && m > THRESH && m < LEN)
	    {
	      if (frag_count == 3)
		frag_count = 1;
	      else
		frag_count += 1;

	      printf ("%s %s\n", w1, w2);
	    }
	}
    }

#if 0
  for (i = 0; example_no < COUNT; i = (i + 1) % word_no)
    {
      int k = 0, m = 0, f = 0;
      int in_match = TRUE;
      char* w;

      w = example[i];

      if (buf[0] == w[0])
	continue;

      for (; k < LEN; k += 1)
	{
	  if (buf[k] == w[k])
	    {
	      if (! in_match)
		f += 1;

	      m += 1;
	      in_match = TRUE;
	    }
	  else
	    in_match = FALSE;
	}

      if (m > 2)
	printf ("m is %d\n", m);

      if (m >= THRESH && m < LEN && f == frag_count)
	{
	  printf ("frag_count was: %d\n", frag_count);
	  frag_count = 1 + (frag_count + 1) % 2;
	  printf ("frag_count now: %d\n\n", frag_count);
	}
    }
#endif
  exit (0);
}
