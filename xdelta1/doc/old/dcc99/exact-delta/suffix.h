/* -*-Mode: c++;-*- */

/* $Header: /home/tully/hilfingr/work/delta/RCS/suffix.h,v 1.6 1995/08/11 07:24:10 hilfingr Exp $								*/
/* suffix.h: Definitions for suffix-tree data type. 			*/

/* Copyright (C) 1995, Paul N. Hilfinger.  All Rights Reserved.		*/


#ifndef _SUFFIX_H_
#define _SUFFIX_H_

#include <assert.h>

typedef unsigned char byte;

class Edge;			/* Edge between two nodes. */

void* allocate(int size);

class SuffixTree {
    /* A suffix tree is the set of all suffixes of a particular string. */
public:
    /* The set { "" } (all suffixes of the empty string). Sets */
    /* string() to "" and length() to 0. */
    SuffixTree();

    SuffixTree(const SuffixTree& T);

    ~SuffixTree();

    /* Set *this to a suffix tree for S[0..L-1]. */
    /* The string S must continue to exist and its contents remain */
    /* unchanged until the next application of init or until *this is */
    /* destroyed. Sets string() to S and length() to L. */
    void init(const byte* S, int/*waslong*/ L);

    /* See definitions of default constructor and init (above). */
    const byte* string() const;
    const int/*waslong*/ length() const;

    /* Set LENGTH to the length of the longest prefix of S1[0..N1-1] that */
    /* occurs in string(), and START to an index in S1 of the */
    /* beginning of that prefix. */
    void longestPrefix(const byte* S1, int/*waslong*/ N1,
		       int/*waslong*/& start, int/*waslong*/& length) const;
private:

    /* See suffix.cc for an explanation of the data structure. */

    typedef int/*waslong*/ NodeNum;
    typedef int/*waslong*/ Length;

    /* Extended bytes: Includes the notional end-of-string character. */
    typedef int xbyte;

    const byte* theString;
				/* Value of string(). */

    int N;			/* strlen(theString)+1. */

    Length* inArcLen;		/* inArcLen[k] is the length of the */
				/* string labeling the arc leading to */
				/* nonterminal node k. */

    Edge* edges;		/* Array of all edges */
    Edge* nextEdge;		/* Next free edge in edges */
    Edge** edgeMap;		/* Hash table: NodeNum x char -> NodeNum. */

    /* See definitions in suffix.cc */

    Length suffixLength(NodeNum m) const;
    bool isTerminal(NodeNum m) const;
    int toStep(NodeNum m) const;
    NodeNum toLeaf(int m) const;
    Length inLength(NodeNum m, Length L) const;
    NodeNum nextNode(NodeNum src, xbyte c) const;
    int suffixStart(NodeNum m) const;
    void splitEdge(Edge* e, NodeNum n2, Length L0, Length L2);
    void addLeaf(NodeNum n0, NodeNum n1, Length L);
    void createEdge(NodeNum n0, NodeNum n1, xbyte x);

    void createSuffixTree();
};

inline const byte* SuffixTree::string() const
{
    return theString;
}

void bail (void);

inline const int/*waslong*/ SuffixTree::length() const
{
    return N-1;
}

typedef SuffixTree::NodeNum NodeNum;
typedef SuffixTree::xbyte xbyte;
typedef SuffixTree::Length Length;

class Edge {
public:
  Edge() {}
  Edge(NodeNum from0, NodeNum to0, int next0)
    : from(from0), to(to0), next(next0)
  {
    assert (to0 >= 0);
    assert (from0 >= 0);
  }

  NodeNum from, to;	/* Source and sink nodes. */

  int next;
};


#endif
