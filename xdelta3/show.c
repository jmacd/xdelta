#include <stdio.h>
#include <stdlib.h>

#define BUFSZ (1 << 22)

int main(int argc, char **argv)
{
  int c;
  int offset;
  int bytes;

  if (argc != 3)
    {
      fprintf (stderr, "usage: show offset bytes\n");
      return 1;
    }

  offset = atoi (argv[1]);
  bytes  = atoi (argv[2]);

  for (; offset != 0; offset -= 1)
    {
      if ((c = fgetc (stdin)) == EOF)
	{
	  fprintf (stderr, "EOF before offset\n");
	}
    }

  for (; bytes != 0; bytes -= 1)
    {
      if ((c = fgetc (stdin)) == EOF)
	{
	  fprintf (stderr, "\nEOF before offset + bytes\n");
	}

      fprintf (stderr, "%02x", c);
    }

  fprintf (stderr, "\n");
  return 0;
}
