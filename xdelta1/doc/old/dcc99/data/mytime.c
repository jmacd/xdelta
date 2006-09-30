#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

int main(int argc, char** argv)
{
  int pid, npid, status = 0;
  char* argv1 = argv[1];

  argv += 2;
  argc -= 2;

  if ((pid = fork()))
    {
      struct rusage ru;
      long t = 0;

      if (pid != (npid = wait4 (pid, &status, 0, &ru)))
	{
	  fprintf (stderr, "wait failed: %d\n", npid);
	  abort ();
	}

      if (! WIFEXITED(status))
	{
	  fprintf (stderr, "process did not exit\n");
	  exit (1);
	}

      if (WEXITSTATUS(status) == 127)
	{
	  fprintf (stderr, "exec failed\n");
	  exit (1);
	}

      if (WEXITSTATUS(status) > 1)
	{
	  fprintf (stderr, "process exited %d\n", WEXITSTATUS(status));
	  exit (1);
	}

      t += 1000000*ru.ru_utime.tv_sec;
      t += 1000000*ru.ru_stime.tv_sec;
      t += ru.ru_utime.tv_usec;
      t += ru.ru_stime.tv_usec;

      fprintf (stderr, "Time: %s %f\n", argv1, (double)t / 1000000.0);

      exit (0);
    }
  else
    {
      execvp (argv[0], argv);
      _exit (127);
    }
  exit (0);
}
