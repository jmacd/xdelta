/* -*- C -*- */

%{
#include <glib.h>
#include <string.h>
#include <math.h>


#define BIG 1

void fromfile();
void tofile();
void tool();
void normal();
void reverse();
void onetime();

char* _from;
int _fromsize;
char* _to;
int _tosize;
char* _tool;

GArray* runs;
GArray* pairs;

#define FORWARD 0
#define REVERSE 1

typedef struct {
  char* from;
  int   from_size;

  char* to;
  int   to_size;

  char* tool;

  double  forward_delta_size;
  double  reverse_delta_size;

  double   forward_delta_time;
  double   reverse_delta_time;

  gboolean missing_time;
} Run;

#define PAIRS 6

#define BUCKET_SIZE 10000

typedef struct {
  Run* runs[PAIRS];
  int  run_count;
} Pair;

int time_index;
int time_stage;
GSList** buckets[PAIRS];

%}

dig       [0-9]
num       {dig}+("."{dig}+)?
space     [\v\f\r\t\n ]
ws        {space}+
filename  [^\v\f\r\t\n ]+

%%

{ws} { }
"FromFile:".* { fromfile(); }
"ToFile:".* { tofile(); }
"Tool:".* { tool(); }
"Normal:".* { normal(); }
"Reverse:".* { reverse(); }
"Time:".* { onetime(); }

%%

char*
first_nonspace (char* p)
{
  while (*p == ' ')
    p += 1;

  return p;
}

void
fromfile()
{
  char** toks = g_strsplit (first_nonspace(yytext + strlen("FromFile:")), " ", -1);

  _from = g_strdup(toks[0]);
  _fromsize = atoi (toks[2]);

  g_free (toks);
}

void
tofile()
{
  char** toks = g_strsplit (first_nonspace(yytext + strlen("ToFile:")), " ", -1);

  _to = g_strdup(toks[0]);
  _tosize = atoi(toks[2]);

  g_free (toks);
}

void
tool()
{
  char** toks = g_strsplit (first_nonspace(yytext + strlen("Tool:")), " ", -1);

  _tool = g_strdup (toks[0]);

  g_free (toks);
}

void
normal()
{
  char** toks = g_strsplit (first_nonspace(yytext + strlen("Normal:")), " ", -1);
  Run r;

  g_assert (toks[0]);

  r.forward_delta_size = atoi (toks[0]);

  r.from = _from;
  r.from_size = _fromsize;

  r.to = _to;
  r.to_size = _tosize;

  r.tool = _tool;

  g_array_append_val(runs, r);

  g_free (toks);
}

void
reverse()
{
  char** toks = g_strsplit (first_nonspace(yytext + strlen("Reverse:")), " ", -1);
  int size = atoi (toks[0]);

  g_array_index (runs, Run, runs->len-1).reverse_delta_size = size;

  g_free (toks);
}

void
onetime()
{
  char** toks = g_strsplit (first_nonspace(yytext + strlen("Time:")), " ", -1);
  char** names = g_strsplit (toks[0], "-", -1);
  char* tool = names[0];
  char* dir = names[1];

  Run* r = &g_array_index (runs, Run, time_index);

  double t;

  int time_stage_max;

  sscanf (toks[1], "%lf", &t);

  g_assert (strncmp (tool, r->tool, 2) == 0);

  if (strncmp (tool, "dz", 2) == 0)
    time_stage_max = 4;
  else
    time_stage_max = 2;

  if (strcmp (dir, "forward") == 0)
    r->forward_delta_time += t;
  else if (strcmp (dir, "reverse") == 0)
    r->reverse_delta_time += t;
  else
    r->missing_time = TRUE;

  if (time_stage < (time_stage_max-1))
    {
      time_stage += 1;
    }
  else
    {
      time_stage = 0;
      time_index += 1;
    }
}

void
add (int j, int size, double* dsize)
{
  g_assert (*dsize > 0.0);
  buckets[j][size / BUCKET_SIZE] = g_slist_prepend (buckets[j][size / BUCKET_SIZE], dsize);
}

void
compute_stats (GSList* dlist, double* mean, double* min, double* max)
{
  GSList* dlist0 = dlist;
  gint c = 0;

  double m, s = 0, s2 = 0;

  /*(*max) = 0.0;
  (*min) = 1000000000.0;*/

  for (; dlist; dlist = dlist->next)
    {
      double d = * (double*) dlist->data;

      g_assert (d > 0.0);

      s += d;
      c += 1;

      /*(*min) = MIN (*min, d);
      (*max) = MAX (*max, d);*/
    }

  m = (*mean) = s / (double) c;

  g_assert (m > 0.0);

  if (c == 1)
    {
      /*(*stddev) = 0.0;*/
      (*min) = m;
      (*max) = m;
    }
  else
    {
      for (dlist = dlist0; dlist; dlist = dlist->next)
	{
	  double d = * (double*) dlist->data;
	  double ds = d-m;

	  s2 += ds*ds;
	}

      s2 /= (double) (c-1);

      /*(*stddev) = sqrt (s2);*/

      (*min) = MIN(sqrt (s2), m);
      (*max) = sqrt (s2);
    }
}

double*
copyd (double d)
{
  double* dp = g_new (double, 1);

  *dp = d;

  return dp;
}

#if 0
void
test()
{
  double avg, std;
  GSList* l = NULL;

  l = g_slist_prepend (l, copyd (3));
  l = g_slist_prepend (l, copyd (4));
  l = g_slist_prepend (l, copyd (5));
  l = g_slist_prepend (l, copyd (6));
  l = g_slist_prepend (l, copyd (7));
  l = g_slist_prepend (l, copyd (8));

  compute_stats (l, &avg, &std);

  fprintf (stderr, "test %f %f\n", avg, std);
}
#endif

int main()
{
  FILE* one = fopen ("runtest.out", "r");
  FILE* two = fopen ("runtest.errs", "r");
  GHashTable* pair_table = g_hash_table_new (g_str_hash, g_str_equal);
  GPtrArray* pair_array = g_ptr_array_new ();
  int i;
  int skipped = 0;
  int max_size = 0;

  g_assert (one && two);

  runs = g_array_new (FALSE, TRUE, sizeof(Run));
  pairs = g_array_new (FALSE, TRUE, sizeof(Pair));

  yyin = one;
  while (yylex ()) ;
  yyin = two;
  while (yylex ()) ;

  /*printf ("%d runs found\n", runs->len);
  printf ("%d runs timed\n", time_index);*/

  for (i = 0; i < runs->len; i += 1)
    {
      Run* r = &g_array_index (runs, Run, i);
      Pair* p;

      if (strcmp (r->tool, "cim") != 0)
	{
	  continue;
	}

      if (r->missing_time)
	{
	  skipped += 1;
#ifndef BIG
	  continue;
#endif
	}

      p = g_new0 (Pair, 1);

      g_ptr_array_add (pair_array, p);
      g_hash_table_insert (pair_table, r->from, p);

      p->run_count = 1;
      p->runs[0] = r;

      max_size = MAX(max_size, r->to_size);
      max_size = MAX(max_size, r->from_size);
    }

  /*printf ("%d runs skipped\n", skipped);*/

  for (i = 0; i < runs->len; i += 1)
    {
      Run* r = &g_array_index (runs, Run, i);
      Pair* p;

      if (strcmp (r->tool, "cim") == 0)
	continue;

      p = g_hash_table_lookup (pair_table, r->from);

      if (! p)
	continue;

      p->runs[p->run_count++] = r;
    }

#ifndef BIG
  g_assert (pair_array->len == (runs->len / PAIRS) - skipped);
#endif

  for (i = 0; i < PAIRS; i += 1)
    buckets[i] = g_new0 (GSList*, max_size / BUCKET_SIZE);

  for (i = 0; i < pair_array->len; i += 1)
    {
      Pair* p = pair_array->pdata[i];
      int j;
      double cim_reverse_size = p->runs[0]->reverse_delta_size;
      double cim_forward_size = p->runs[0]->forward_delta_size;

#ifdef BIG
      if (p->runs[0]->missing_time)
	{
	  printf ("%d, %f, %f\n", p->runs[0]->to_size, p->runs[1]->forward_delta_size, p->runs[2]->forward_delta_size);
	  printf ("%d, %f, %f\n", p->runs[0]->from_size, p->runs[1]->reverse_delta_size, p->runs[2]->reverse_delta_size);
	}
#else

      if (cim_reverse_size == 0.0)
	continue;
      if (cim_forward_size == 0.0)
	continue;

      for (j = 0; j < PAIRS; j += 1)
	{
	  if (p->runs[j]->tool[0] == 'x')
	    {
	      /* take out the xdelta header */
	      int len = strlen (p->runs[j]->from) + strlen (p->runs[j]->to);

	      p->runs[j]->forward_delta_size -= 28 + len;
	      p->runs[j]->reverse_delta_size -= 28 + len;

	      g_assert (p->runs[j]->reverse_delta_size > 0.0);
	      g_assert (p->runs[j]->forward_delta_size > 0.0);
	    }

	  p->runs[j]->forward_delta_size /= cim_forward_size;
	  p->runs[j]->reverse_delta_size /= cim_reverse_size;

	  add (j, p->runs[0]->to_size,   &p->runs[j]->forward_delta_size);
	  add (j, p->runs[0]->from_size, &p->runs[j]->reverse_delta_size);
	}
    }

  for (i = 0; i < max_size / BUCKET_SIZE; i += 1)
    {
      int j, found = 0;

      for (j = 0; j < PAIRS; j += 1)
	{
	  if (buckets[j][i])
	    {
	      printf ("%d, ", (i * BUCKET_SIZE) + BUCKET_SIZE/2);
	      found = 1;
	      break;
	    }
	}

      if (found)
	{
	  for (j = 0; j < PAIRS; j += 1)
	    {
	      double avg, min, max;

	      compute_stats (buckets[j][i], &avg, &min, &max);

	      printf ("%f, %f, %f, ", avg, min, max);
	    }

	  printf ("\n");
	}
#endif
    }

  return 0;
}
