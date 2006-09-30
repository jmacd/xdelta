/* -*- Mode: C;-*-
 *
 * This file is part of XDelta - A binary delta generator.
 *
 * Copyright (C) 1997, 1998, 2001  Josh MacDonald
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 *
 * $Id: xdeltatest.c 1.6 Fri, 29 Jun 2001 06:01:08 -0700 jmacd $
 */

#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#include "config.h"

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <stdio.h>
#include <zlib.h>

#include "xdelta.h"

//////////////////////////////////////////////////////////////////////
// Configure these parameters
// @@@ Of course, a real test wouldn't require this configuration,
// fix that!
//////////////////////////////////////////////////////////////////////

const char* cmd_data_source      = "/zerostreet2/small-scratch-file";
guint       cmd_size             = 1<<20;

guint       cmd_warmups          = 2;
guint       cmd_reps             = 20;

guint       cmd_changes          = 1000;
guint       cmd_deletion_length  = 30;
guint       cmd_insertion_length = 15;

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////

#define ARRAY_LENGTH(x) (sizeof(x)/sizeof(x[0]))

#define TEST_PREFIX "/tmp/xdeltatest"

#define TEST_IS_GZIP   1
#define TEST_IS_XDELTA 2

typedef struct _File        File;
typedef struct _Patch       Patch;
typedef struct _TestProfile TestProfile;
typedef struct _Instruction Instruction;

struct _Instruction {
  guint32 offset;
  guint32 length;
  Instruction* next;
};

struct _File
{
  char name[MAXPATHLEN];
};

struct _Patch
{
  guint8  *data;
  guint32  length;
};

struct _TestProfile
{
  const char *name;
  const char *progname;
  int         flags;
};

#undef  BUFSIZ
#define BUFSIZ (1<<24)

guint8 __tmp_buffer[BUFSIZ];

// Note: Of course you should be able to set all this up on the
// command line.

TestProfile cmd_profiles[] =
{
  { "xdelta -qn   ", "../xdelta",     TEST_IS_XDELTA },
  { "diff --rcs -a", "/usr/bin/diff", 0 },
  { "gzip         ", "/bin/gzip",     TEST_IS_GZIP },
};

int         cmd_slevels[] = { 16, 32, 64 };
int         cmd_zlevels[] = { 0, 1, 3, 6, 9 };
guint16     cmd_seed[3]          = { 47384, 8594, 27489 };

FILE*       data_source_handle;
guint       data_source_length;
File*       data_source_file;

long        current_to_size;
long        current_from_size;

double      __total_time;
off_t       __dsize;

void
reset_stats ()
{
  __total_time = 0;
  __dsize      = -1;
}

void add_tv (GTimer *timer)
{
  __total_time += g_timer_elapsed (timer, NULL);
}

void
report (TestProfile *tp, int zlevel, int slevel)
{
  static gboolean once = TRUE;

  double t = __total_time / (double) cmd_reps;
  double s;
  const char *u;
  char  slevel_str[16];

  if (tp->flags & TEST_IS_XDELTA)
    {
      sprintf (slevel_str, "%d", slevel);
    }
  else
    {
      slevel_str[0] = 0;
    }

  if (tp->flags & TEST_IS_GZIP)
    {
      s = __dsize / (double) current_to_size;
      u = "total size";
    }
  else
    {
      s = __dsize / (double) (cmd_changes * cmd_insertion_length);
      u = "insertion size";
    }

  if (once)
    {
      once = FALSE;

      g_print ("Program\t\tzlevel\tslevel\tSeconds\tNormalized\n\t\t\t\t\tcompression\n");
    }

  g_print ("%s\t%d\t%s\t%0.3f\t%0.3f (of %s)\n", tp->name, zlevel, slevel_str, t, s, u);
}

guint32
random_offset (guint len)
{
  return lrand48 () % (data_source_length - len);
}

void
fail ()
{
  g_warning ("FAILURE\n");
  abort ();
}

gboolean starts_with (const char* s, const char *start)
{
  return strncmp (s, start, strlen (start)) == 0;
}

Patch*
read_patch (File *file, struct stat *sbuf)
{
  Patch *p = g_new0 (Patch,1);
  FILE  *f = fopen (file->name, "r");

  p->length = (int)sbuf->st_size;

  p->data   = g_malloc (p->length);

  if (! f || fread (p->data, 1, p->length, f) != p->length)
    {
      perror ("fread");
      fail ();
    }

  fclose (f);

  return p;
}

File*
empty_file ()
{
  static gint count = 0;
  File *file = g_new0 (File, 1);

  sprintf (file->name, "%s.%d.%d", TEST_PREFIX, getpid (), count++);

  return file;
}

void
compare_files (File* fromfile, File* tofile)
{
  gint pos = 0;

  FILE* toh;
  FILE* fromh;

  guint8 buf1[1024], buf2[1024];

  toh   = fopen (tofile->name, "r");
  fromh = fopen (fromfile->name, "r");

  if (!toh || !fromh)
    fail ();

  for (;;)
    {
      gint readf = fread (buf1, 1, 1024, fromh);
      gint readt = fread (buf2, 1, 1024, toh);
      gint i, m = MIN(readf, readt);

      if (readf < 0 || readt < 0)
	fail ();

      for (i = 0; i < m; i += 1, pos += 1)
	{
	  if (buf1[i] != buf2[i])
	    fail ();
	}

      if (m != 1024)
	{
	  if (readt == readf)
	    {
	      fclose (toh);
	      fclose (fromh);
	      return;
	    }

	  fail ();
	}
    }
}

int
write_file (File* file, Instruction* inst)
{
  FILE* h;
  int ret;
  int size = 0;

  if (! (h = fopen (file->name, "w"))) {
    perror (file->name);
    fail ();
  }

  for (; inst; inst = inst->next)
    {
      g_assert (inst->length <= BUFSIZ);

      if ((ret = fseek (data_source_handle, inst->offset, SEEK_SET))) {
	perror ("fseek");
	fail ();
      }

      if ((ret = fread (__tmp_buffer, 1, inst->length, data_source_handle)) != inst->length) {
	perror ("fread");
	fail ();
      }

      if ((ret = fwrite (__tmp_buffer, 1, inst->length, h)) != inst->length) {
	perror ("fwrite");
	fail ();
      }

      size += inst->length;
    }

  if ((ret = fclose (h))) {
    perror ("fclose");
    fail ();
  }

  return size;
}

Instruction*
perform_change_rec (Instruction* inst, guint32 change_off, guint* total_len)
{
  if (change_off < inst->length)
    {
      guint32 to_delete = cmd_deletion_length;
      guint32 avail = inst->length;
      guint32 this_delete = MIN (to_delete, avail);
      Instruction* new_inst;

      // One delete
      inst->length -= this_delete;
      to_delete -= this_delete;

      while (to_delete > 0 && inst->next->length < to_delete)
	{
	  to_delete -= inst->next->length;
	  inst->next = inst->next->next;
	}

      if (to_delete > 0)
	inst->next->offset += to_delete;

      // One insert
      new_inst = g_new0 (Instruction, 1);

      new_inst->offset = random_offset (cmd_insertion_length);
      new_inst->length = cmd_insertion_length;
      new_inst->next = inst->next;
      inst->next = new_inst;

      (* total_len) += cmd_insertion_length - cmd_deletion_length;

      return inst;
    }
  else
    {
      inst->next = perform_change_rec (inst->next, change_off - inst->length, total_len);
      return inst;
    }
}

Instruction*
perform_change (Instruction* inst, guint* len)
{
  return perform_change_rec (inst, lrand48() % ((* len) - cmd_deletion_length), len);
}

gboolean
xdelta_verify (TestProfile *tp, int zlevel, int slevel, File* from, File* to, File* out, File *re)
{
  int pid, status;

  if ((pid = vfork()) < 0)
    return FALSE;

  if (pid == 0)
    {
      execl (tp->progname,
	     tp->progname,
	     "patch",
	     out->name,
	     from->name,
	     re->name,
	     NULL);
      perror ("execl failed");
      _exit (127);
    }

  if (waitpid (pid, &status, 0) != pid)
    {
      perror ("wait failed");
      fail ();
    }

  // Note: program is expected to return 0, 1 meaning diff or no diff,
  // > 1 indicates failure
  if (! WIFEXITED (status) || WEXITSTATUS (status) > 1)
    {
      g_warning ("patch command failed\n");
      fail ();
    }

  compare_files (to, re);

  return TRUE;
}

gboolean
run_command (TestProfile *tp, int zlevel, int slevel, File* from, File* to, File* out, File *re, gboolean accounting)
{
  int pid, status, outfd;
  struct stat sbuf;
  char xdelta_args[16];
  char xdelta_args2[16];
  char diff_gzargs[16];
  char gzip_args[16];

  GTimer *timer = g_timer_new ();

  sprintf (xdelta_args, "-qn%d", zlevel);
  sprintf (xdelta_args2, "-s%d", slevel);
  sprintf (diff_gzargs, "wb%d", zlevel);
  sprintf (gzip_args, "-c%d", zlevel);

  unlink (out->name);

  g_timer_start (timer);

  if ((pid = vfork()) < 0)
    return FALSE;

  if (pid == 0)
    {
      if (starts_with (tp->name, "xdelta"))
	{
	  execl (tp->progname,
		 tp->progname,
		 "delta",
		 xdelta_args,
		 xdelta_args2,
		 from->name,
		 to->name,
		 out->name,
		 NULL);
	}
      else
	{
	  outfd = open (out->name, O_CREAT | O_TRUNC | O_WRONLY, 0777);

	  if (outfd < 0)
	    {
	      perror ("open");
	      fail ();
	    }

	  dup2(outfd, STDOUT_FILENO);

	  if (close (outfd))
	    {
	      perror ("close");
	      fail ();
	    }

	  if (starts_with (tp->name, "diff"))
	    execl (tp->progname,
		   tp->progname,
		   "--rcs",
		   "-a",
		   from->name,
		   to->name,
		   NULL);
	  else if (starts_with (tp->name, "gzip"))
	    execl (tp->progname,
		   tp->progname,
		   gzip_args,
		   to->name,
		   NULL);
	  else
	    abort ();
	}

      perror ("execl failed");
      _exit (127);
    }

  if (waitpid (pid, &status, 0) != pid)
    {
      perror ("wait failed");
      fail ();
    }

  // Note: program is expected to return 0, 1 meaning diff or no diff,
  // > 1 indicates failure
  if (! WIFEXITED (status) || WEXITSTATUS (status) > 1)
    {
      g_warning ("delta command failed\n");
      fail ();
    }

  if (stat (out->name, & sbuf))
    {
      perror ("stat");
      fail ();
    }

  // Do zlib compression on behalf of diff
  if (zlevel > 0 && starts_with (tp->name, "diff"))
    {
      Patch  *patch   = read_patch (out, & sbuf);
      gzFile *rewrite = gzopen (out->name, diff_gzargs);

      if (! rewrite) fail ();

      if (gzwrite (rewrite, patch->data, patch->length) == 0) { perror ("gzwrite"); fail (); }
      if (gzclose (rewrite) != Z_OK)                          { perror ("gzclose"); fail (); }
      if (stat    (out->name, & sbuf))                        { perror ("stat");    fail (); }
    }

  g_timer_stop (timer);

  if (accounting)
    {
      add_tv (timer);
    }

  if (__dsize < 0)
    {
      __dsize = sbuf.st_size;

      // Verify only once

      if (starts_with (tp->name, "xdelta"))
	{
	  if (! xdelta_verify (tp, zlevel, slevel, from, to, out, re))
	    {
	      g_warning ("verify failed");
	      fail ();
	    }
	}
    }
  else
    {
      if (__dsize != sbuf.st_size)
	{
	  g_warning ("%s -%d: delta command produced different size deltas: %d and %d",
		     tp->progname, zlevel, (int)__dsize, (int)sbuf.st_size);
	  fail ();
	}
    }

  g_timer_destroy (timer);

  return TRUE;
}

void
test1 (TestProfile *test_profile,
       File        *from_file,
       File        *to_file,
       File        *out_file,
       File        *re_file)
{
  int ret;
  guint i, change, current_size = cmd_size;
  guint end_size = (cmd_changes * cmd_insertion_length) + cmd_size;
  Instruction* inst;
  struct stat sbuf;
  int zlevel_i, slevel_i;

  seed48 (cmd_seed);

  if ((ret = stat (cmd_data_source, & sbuf)))
    {
      perror (cmd_data_source);
      fail ();
    }

  if (! (data_source_handle = fopen (cmd_data_source, "r")))
    {
      perror (cmd_data_source);
      fail ();
    }

  data_source_length = sbuf.st_size;

  /* arbitrary checks */
  if (data_source_length < (1.5 * end_size))
    g_warning ("data source should be longer\n");

  if ((cmd_changes * cmd_deletion_length) > cmd_size)
    {
      g_warning ("no copies are expected\n");
      fail ();
    }

  inst = g_new0 (Instruction, 1);

  inst->offset = random_offset (cmd_size);
  inst->length = cmd_size;

  current_from_size = write_file (from_file, inst);

  for (change = 0; change < cmd_changes; change += 1)
    inst = perform_change (inst, & current_size);

  current_to_size = write_file (to_file, inst);

  for (slevel_i = 0; slevel_i < ARRAY_LENGTH(cmd_slevels); slevel_i += 1)
    {
      int slevel = cmd_slevels[slevel_i];

      if ((test_profile->flags & TEST_IS_XDELTA) == 0 && slevel_i != 0)
	{
	  continue;
	}

      for (zlevel_i = 0; zlevel_i < ARRAY_LENGTH(cmd_zlevels); zlevel_i += 1)
	{
	  int zlevel = cmd_zlevels[zlevel_i];

	  if (test_profile->flags & TEST_IS_GZIP)
	    {
	      if (zlevel != 1 && zlevel != 9)
		continue;
	    }

	  reset_stats ();

	  for (i = 0; i < cmd_warmups + cmd_reps; i += 1)
	    {
	      if (! run_command (test_profile,
				 zlevel,
				 slevel,
				 from_file,
				 to_file,
				 out_file,
				 re_file,
				 (i >= cmd_warmups) /* true => accounting */))
		{
		  fail ();
		}
	    }

	  report (test_profile, zlevel, slevel);
	}

      g_print ("\n");
    }
}

int
main (gint argc, gchar** argv)
{
  int profile_i;

  File *from_file;
  File *to_file;
  File *out_file;
  File *re_file;

  system ("rm -rf " TEST_PREFIX "*");

  from_file = empty_file ();
  to_file   = empty_file ();
  out_file  = empty_file ();
  re_file   = empty_file ();

  for (profile_i = 0; profile_i < ARRAY_LENGTH(cmd_profiles); profile_i += 1)
    {
      test1 (& cmd_profiles[profile_i], from_file, to_file, out_file, re_file);

      system ("rm -rf " TEST_PREFIX "*");
    }

  return 0;
}
