#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <ctype.h>

GHashTable* node_table;
GStringChunk* chunk;

typedef struct _Node Node;
typedef struct _IntPair IntPair;
typedef struct _Edge Edge;

struct _IntPair {
  int d1;
  int d2;
};

struct _Node {
  int mark[3];
  int finish;
  char *name;
  Edge *edges;
};

struct _Edge {
  char* pict;
  int   frags;
  int   copy;
  Node* node;
  Edge* next;
};

Node* get_node (char* name)
{
  Node* n = g_hash_table_lookup (node_table, name);

  if (n)
    return n;

  n = g_new0 (Node, 1);
  n->name = name;

  g_hash_table_insert (node_table, name, n);

  return n;
}

void connect (Node* n1, Node* n2)
{
  Edge* nedge = g_new0 (Edge, 1);

  nedge->node = n1;
  nedge->next = n2->edges;
  n2->edges = nedge;

  nedge = g_new0 (Edge, 1);

  nedge->node = n2;
  nedge->next = n1->edges;
  n1->edges = nedge;
}

int global_count = 0;
int global_e_count = 0;
int global_depth = 0;
int global_fragments = 0;
int global_longest = 0;
int global_longest2 = 0;

void
count_fragments (Node* n1, Node* n2, Edge* e)
{
  int i, l = strlen (n1->name);
  int in_match = TRUE;
  int m = 0, f = 0;
  char buf1[16];

  memset (buf1, ' ', l);

  buf1[l] = 0;

  for (i = 0; i < l; i += 1)
    {
      if (n1->name[i] == n2->name[i])
	{
	  if (! in_match)
	    {
	      global_fragments += 1;
	      f += 1;
	    }

	  in_match = TRUE;
	  m += 1;
	  buf1[i] = 'x';
	}
      else
	in_match = FALSE;
    }

  e->pict = g_strdup (buf1);
  e->copy = m;
  e->frags = f;
}

int
dfs (Node* n, gint phase, gint print, gint depth)
{
  int d1 = 0, d2 = 0, c;
  Edge* e;

  global_depth = MAX(depth, global_depth);

  if (n->mark[phase])
    {
      return 1;
    }

  n->mark[phase] = TRUE;

  for (e = n->edges; e; e = e->next)
    {
      int od1;

      global_e_count += 1;

      count_fragments (n, e->node, e);

      c = dfs (e->node, phase, print, depth + 1);

      od1 = d1;
      d1 = MAX (d1, c);

      if (d1 == od1)
	{
	  d2 = MAX (d2, c);
	}
      else
	d2 = od1;
    }

  global_count += 1;

  if (print)
    {
      g_print ("%s ->\n", n->name);

      for (e = n->edges; e; e = e->next)
	g_print ("  %s %s %d %d\n", e->node->name, e->pict, e->copy, e->frags);

      g_print ("\n\n");
    }

  if (! print && depth == 0 && d1 + d2 == 8)
    {
      g_print ("found a graph of size nodes=%d, edges=%d, depth=%d, frags=%d:\n", global_count, global_e_count, d1+d2, global_fragments);

      dfs (n, 1, TRUE, 0);

      g_print ("\n");
      g_print ("\n");
    }

  return d1 + 1;
}

void
find (gpointer	key,
      gpointer	value,
      gpointer	user_data)
{
  Node* n = value;

  global_fragments = 0;
  global_count = 0;
  global_e_count = 0;
  global_depth = 0;

  dfs (n, 0, FALSE, 0);

  global_e_count /= 2;

  if (global_count == 0)
    return;

  /*g_assert (global_count != global_e_count);*/

  if (FALSE && global_depth > 7)
    {
      g_print ("found a graph of size nodes=%d, edges=%d, depth=%d, %d:\n", global_count, global_e_count, global_depth, global_fragments);

      dfs (n, 1, TRUE, 0);

      g_print ("\n");
      g_print ("\n");
    }
}

int main()
{
  char from_buf[2048];
  char to_buf[2048];

  node_table = g_hash_table_new (g_direct_hash, g_direct_equal);
  chunk = g_string_chunk_new (1024);

  while (scanf ("%s %s", from_buf, to_buf) == 2)
    {
      Node* nfrom;
      Node* nto;
      Edge* e;

      char* from = g_string_chunk_insert_const (chunk, from_buf);
      char* to = g_string_chunk_insert_const (chunk, to_buf);

      nfrom = get_node (from);
      nto = get_node (to);

      if (! nfrom->edges)
	{
	  connect (nfrom, nto);
	  continue;
	}

      for (e = nfrom->edges; e; e = e->next)
	{
	  if (e->node == nto)
	    {
	      break;
	    }

	  if (e->next == NULL)
	    {
	      connect (nfrom, nto);
	    }
	}
    }

  g_hash_table_foreach (node_table, find, NULL);

  exit (1);
}
