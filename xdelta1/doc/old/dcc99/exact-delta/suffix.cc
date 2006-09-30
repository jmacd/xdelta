/* $Header: /home/tully/hilfingr/work/delta/RCS/suffix.cc,v 1.6 1995/08/11 07:24:10 hilfingr Exp hilfingr $								*/
/* suffix.cc:								*/

/* Copyright (C) 1995, Paul N. Hilfinger.  All Rights Reserved.		*/

#include <iostream.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "suffix.h"

/* This file has been grossly hacked by Josh. */

/* Notionally, the string theString has appended to it a sentinel */
/* character that is unequal to any other character.  This value can't */
/* be represented directly in a byte, however, so we must take care */
/* throughout to avoid actually trying to look at or compare the */
/* sentinel character directly. */

/* Each SuffixTree contains nonterminal nodes identified by integers	*/
/* in the range 0 .. N and terminal nodes identified by integers in	*/
/* the range N+1 .. 2N.  Each node represents a string, and is said to	*/
/* be the LOCUS of that string.  Nonterminal node 0 represents the	*/
/* null string.  Nonterminal node k>1, if it exists, is the locus of	*/
/* the longest prefix of theString[k-1 .. N-1] that is present in	*/
/* theString[j..N-1], 0<=j<k-1; there is at most one nonterminal node	*/
/* for each prefix.  The prefix itself (whether or not a nonterminal */
/* node exists for it) is called head(k). Terminal node N+k, k>0, */
/* represents the suffix theString[k-1..N-1].  There is one terminal */
/* node for each suffix.	*/

/* The EXTENDED LOCUS of a string is the locus of the shortest string */
/* in the tree that has that string as a prefix.  The CONTRACTED LOCUS */
/* of a string is the locus of the longest string in the tree that is */
/* a prefix of that string. */

/* Each edge of the tree is labeled with a non-null string such that	*/
/* the concatenation of the strings on all edges leading from the root	*/
/* to a node is the string represented by that node.  For each		*/
/* nonterminal node k, inArcLen[k] is the length of the string		*/
/* labeling the edge incoming to k.  All nonterminal nodes have at	*/
/* least two edges leading from them, each beginning with a distinct	*/
/* character.								*/

/* For nonterminal node m and character c, the hash table edgeMap maps	*/
/* (m,c) to the edge from m whose label begins with c, if there is	*/
/* one.  The member array edges contains all edges, and each edge is	*/
/* represented by its index in edges.  Each bin in edgeMap is linked	*/
/* through the next fields of the edges.  The hash function is such	*/
/* that at most one entry whose source is a given node can appear on a	*/
/* given hash chain.							*/

/* This data structure and the algorithm for createSuffixTree, below, */
/* were originally described in Edward M. McCreight, ``A */
/* Space-Economical Suffix Tree Construction Algorithm'', JACM 23, 2 */
/* (April, 1976), pp 262-272.			*/

const NodeNum Root = 0;
const int TERMINAL_FLAG = 0x40000000;
const xbyte SENTINEL_CHAR = 0x1ff;

void bail (void)
{
  printf ("-1\n");
  exit (2);
}

void* allocate(int size)
{
  void* p = malloc (size);
  static int total = 0;

  total += size;

  if (total >= 1<<30)
    bail ();

  if (! p)
    bail ();

  return p;
}

extern Edge _NULL_EDGE;
Edge _NULL_EDGE(Root, Root, -1);
static Edge* const NULL_EDGE = &_NULL_EDGE;

inline NodeNum hash(xbyte c, NodeNum n)
{
    return (c | (c << 8)) ^ n;
}

#define member SuffixTree::

member SuffixTree()
{
    theString = (byte*) "";
    N = 1;
    inArcLen = NULL;
    nextEdge = NULL;
    edges = NULL;
    edgeMap = NULL;
}

member ~SuffixTree()
{
    delete [] inArcLen;
    delete [] edges;
    delete [] edgeMap;
    inArcLen = NULL; edges = NULL; edgeMap = NULL;
}

void member init(const byte* S, int/*waslong*/ L)
{
    this->~SuffixTree();

    N = L + 1;
    theString = S;

    createSuffixTree();
}

/* The step (in createSuffixTree: see below) at which node M>0 was */
/* created.  Step k>0 considers the suffix beginning at theString[k-1]. */
inline int member toStep(NodeNum m) const
{
    return m & ~TERMINAL_FLAG;
}

/* The leaf node corresponding to the suffix beginning at character M. */
inline NodeNum member toLeaf(int m) const
{
    return m | TERMINAL_FLAG;
}


/* The length of a suffix of which node M is a part.  If M is a */
/* terminal node, there is only one such suffix. */
inline Length member suffixLength(NodeNum m) const
{
    return N - toStep(m) + 1;
}

/* True iff M is a terminal node */
inline bool member isTerminal(NodeNum M) const
{
    return (M & TERMINAL_FLAG) != 0;
}

/* The length of the arc incoming to M, given that L is the length of */
/* the string associated with M's parent. */
inline Length member inLength(NodeNum m, Length L) const
{
    if (isTerminal(m))
	return suffixLength(m) - L;
    else
	return inArcLen[m];
}

/* The starting position in string() of the string associated with */
/* m>0. */
inline int member suffixStart(NodeNum m) const
{
    return toStep(m)-1;
}

/* Create a new edge from n0 to n1 whose label begins with string x. */
inline void member createEdge(NodeNum n0, NodeNum n1, xbyte x)
{
    assert (n0 >= 0 && n1 >= 0);
    assert (nextNode(n0, x) == Root);
    assert (nextEdge - edges < 2*N);
    int h = hash(x, n0);

    if (edgeMap[h] == NULL_EDGE)
      *nextEdge = Edge(n0, n1, -1);
    else
      *nextEdge = Edge(n0, n1, edgeMap[h] - edges);

    edgeMap[h] = nextEdge;

    nextEdge += 1;
}

/* Insert new node N2 on edge E, where the source of E  */
/* represents a string of length L0 and L2 is the length of the string */
/* between the source of E and N2. */
inline void member splitEdge(Edge* e, NodeNum n2, Length L0, Length L2)
{
    NodeNum n0 = e->from, n1 = e->to;

    assert( e != NULL_EDGE && ! isTerminal(n0) &&
	   ! isTerminal(n2) && L2 < inLength(n1, L0));

    assert (n0 >= 0 && n1 >= 0 && n2 >= 0);

    inArcLen[n2] = L2;
    if (! isTerminal(n1))
	inArcLen[n1] -= L2;
    e->to = n2;
    createEdge(n2, n1, theString[suffixStart(n1) + L0 + L2]);
}

/* Insert edge to new leaf node N1 from N0, where N0 represents a */
/* string of length L. */
inline void member addLeaf(NodeNum n0, NodeNum n1, Length L)
{
    assert(isTerminal(n1) && ! isTerminal(n0) && suffixStart(n1)+L < N);
    assert (n0 >= 0);
    assert (n1 >= 0);
    if (suffixStart(n1) + L == N-1)
	createEdge(n0, n1, SENTINEL_CHAR);
    else
	createEdge(n0, n1, theString[suffixStart(n1)+L]);
}

/* The edge from SRC labeled with a string beginning with C, if any, */
/* or NULL_EDGE otherwise. */
inline Edge* edge(NodeNum src, xbyte c, Edge* const edgeMap[], Edge* edges)
{
    Edge* e;

    e = edgeMap[hash(c, src)];

    for (;; e = edges + e->next)
      {
	if (e->from == src)
	  return e;

	if (e->next == -1)
	  return NULL_EDGE;
      }

    return NULL_EDGE;
}

/* Node arrived at by following the arc whose label begins with C, if */
/* any, from nonterminal node SRC.  Returns Root if there is no such */
/* node. */
inline NodeNum member nextNode(NodeNum src, xbyte c) const
{
    return edge(src, c, edgeMap, edges) -> to;
}

void member longestPrefix(const byte* S1, int/*waslong*/ N1,
			  int/*waslong*/& start, int/*waslong*/& length) const
{
    int N0 = N-1;
    int L;			/* Length of S1 scanned so far. */
    NodeNum m;			/* Extended locus of S1[0..L-1].  */
    int r;			/* Characters remaining in current arc */
				/* (incoming to node m). */
    int s;			/* Start of current prefix in S. */

    L = 0; r = 0; m = Root; s = 0;
    while (true) {
	while (r > 0) {
	    if (S1[L] != theString[s+L]) {
		start = s; length = L;
		return;
	    }
	    r -= 1; L += 1;
	}

	if (L == N1 || s+L == N0) {
	    start = s; length = L;
	    return;
	}

	m = nextNode(m, S1[L]);
	if (m == Root) {
	    start = s; length = L;
	    return;
	} else if (isTerminal(m))
	    /* r := remaining length - 2 (so as not to include the */
	    /* sentinel character or the first character on the arc). */
	    r = suffixLength(m) - L - 2;
	else
	    /* r := arc length - 1 (so as not to include the first */
	    /* character on the arc). */
	    r = inArcLen[m] - 1;
	s = suffixStart(m);
	L += 1;			/* Skip first character on arc. */
	if (L+r > N1)
	    r = N1 - L;
    }
}

/* Assuming that N and theString are set correctly, set edgeMap,	*/
/* edges, and inArcLen to represent a tree corresponding to the		*/
/* suffixes of theString, as described above.				*/
void member createSuffixTree()
{
    if (N == 1)
	return;

    inArcLen = (Length*) allocate (sizeof(Length) * (N+1));
    nextEdge = edges = (Edge*) allocate (sizeof(Edge) * (2*N));
    edgeMap = (Edge**) allocate (sizeof(Edge*) * (0x20000+N+1));

    /* For nonterminal node 0<m<k-1 (where k is the current step in the */
    /* algorithm---the step at which node k or node N+k is created), */
    /* with m the locus of string xS (x a character, S a string), */
    /* suffix[m] is the locus of S. */
    NodeNum* suffix = (NodeNum*) allocate (sizeof(NodeNum) * (N+1));
    memset(suffix, '\000', sizeof(NodeNum) * (N+1));

    for (int/*waslong*/ y = 0x20000+N+1; y >= 0; y -= 1)
	edgeMap[y] = NULL_EDGE;
    inArcLen[Root] = 0;

    /* Step 1. */
    addLeaf(Root, toLeaf(1), 0);
    /* The tree now contains the trivial suffix theString[0..N-1]. */

    NodeNum nextPrefix;		/* At the beginning of step k, a */
				/* nonterminal node representing a prefix */
				/* of head(k). */
    int nextPrefixLen;		/* At the beginning of step k, the */
				/* length of the prefix denoted by */
				/* nextPrefix. */
    NodeNum lastHead;		/* At the beginning of step k, the */
				/* locus of head(k-1). */
    int lastHeadLen;		/* At the beginning of step k, the */
				/* length of head(k-1). */


    lastHead = Root; lastHeadLen = 0;
    nextPrefix = Root; nextPrefixLen = 0;
    /* Steps 2 to N. */
    for (NodeNum k = 2; k <= N; k += 1) {
	/* The tree now contains the strings theString[j..N-1], */
	/* 0 <= j< k-1 (call this tree T(k-1)). */
	NodeNum d0, d1;
	int/*waslong*/ r, L;

	/* The suffix theString[k-1..N-1]; */
	const byte* const suff_k = theString+k-1;
	/* Its length. */
	const int/*waslong*/ suff_k_len = N - k;

	/* tail(head(k-1)) == theString[k-1 .. k-3+lastHeadLen] is */
	/* guaranteed to be a prefix of some string currently in the */
	/* tree.  Rescan to find (or if necessary, create) the locus */
	/* of tail(head(k-1)) in the tree, setting d1 to that locus. */
	/* Set nextPrefix and nextPrefixLen for the next iteration. */
	/* The sentinel character is guaranteed not to be a part of */
	/* head(k-1). */

	d0 = d1= nextPrefix;
	L = nextPrefixLen;
	nextPrefix = Root; nextPrefixLen = 0;
	while (L < lastHeadLen-1) {
	    Edge* e = edge(d0, suff_k[L], edgeMap, edges);
	    assert(e != NULL_EDGE);
	    d1 = e->to;
	    assert (d1 != Root);
	    r = inLength(d1, L);
	    if (r + L >= lastHeadLen) {
		r = lastHeadLen - 1 - L;
		splitEdge(e, k, L, r);
		d1 = k;
		break;
	    }
	    L += r;
	    d0 = d1;
	    assert (d1 <= N);
	    nextPrefix = suffix[d1];
	    nextPrefixLen = L-1;
	    assert(! isTerminal(nextPrefix) &&
		   (nextPrefix == Root) == (nextPrefixLen == 0));
	}

	/* Estabish the suffix link for head(k-1). */
	assert (lastHead <= N);
	if (suffix[lastHead] == Root) {
	    assert( !isTerminal(d1));
	    suffix[lastHead] = d1;
	}

	/* Now scan down the tree from d1 until we "fall out" of the */
	/* tree.  Insert a new leaf pointer (to a node representing */
	/* suff_k) at that point, also creating, if necessary, a */
	/* new nonterminal node if we fall out in the middle of an */
	/* edge.  The node from which we insert the new leaf pointer */
	/* is the locus of head(k).  Here, it is possible to run off */
	/* the end of suff_k.  When this happens, we act as if  */
	/* SENTINEL_CHAR were the offending character. */
	L = lastHeadLen == 0 ? 0 : lastHeadLen - 1;
	d0 = d1;
	while (true) {
	    int r0, i;

	    Edge* e = edge(d0, suff_k[L], edgeMap, edges);
	    d1 = e->to;
	    if (d1 == Root)
		break;
	    r0 = inLength(d1, L);

	    if (r0+L > suff_k_len)
		r = suff_k_len - L;
	    else
		r = r0;
	    const byte* const suff_d1 = theString + suffixStart(d1);
	    for (i = 1; i < r && suff_k[L+i] == suff_d1[L+i]; i += 1)
	      {}
	    if (i == r0) {
		L += r;
		d0 = d1;
		assert (d1 <= N);
		nextPrefix = suffix[d1]; nextPrefixLen = L-1;
	    } else {
		splitEdge(e, k, L, i);
		d0 = d1 = k;
		L += i;
		break;
	    }
	}
	addLeaf(d0, toLeaf(k), L);
	lastHead = d0; lastHeadLen = L;
    }

    delete [] suffix;
}
