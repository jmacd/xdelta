/* $Header: /home/tully/hilfingr/work/delta/RCS/delta.cc,v 1.6 1995/08/11 02:06:27 hilfingr Exp $								*/
/* delta.c: Produce a delta indicating how to convert one file to */
/* another. */

/* Copyright (C) 1995, Paul N. Hilfinger.  All Rights Reserved.		*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream.h>
#include <fstream.h>
#include <strstream.h>
#include "suffix.h"

int/*waslong*/ cim = 0;
int/*waslong*/ ilen = 0;

int/*waslong*/
ilog2 (int/*waslong*/ arg)
{
  int/*waslong*/ log = 0;

  while (arg >>= 1) log += 1;

  return log;
}

void
measureInsert (int/*waslong*/ len)
{
  ilen += len;
}

void
measureCopy (int/*waslong*/ len, int/*waslong*/ at)
{
  if (ilen > 0)
    {
      cim += 1 + ilog2 (ilen) + 8*ilen;
#if 0
      printf ("insert %ld %ld\n", ilen, cim);
#endif
      ilen = 0;
    }

  if (len > 0)
    {
      cim += 1 + ilog2 (len) + ilog2(at);
#if 0
      printf ("copy %ld %ld\n", len, cim);
#endif
    }
}

/* Usage: */
/*     delta file1 file2 */
/* Produces on standard output a sequence of commands for converting */
/* the contents of FILE1 to those of FILE2.  Each command has the */
/* following form:  */
/*    <p><n0><n1><x> */
/* where <p> is a signed number, <n0> and <n1> are nonnegative */
/* numbers, and <x> is a sequence of <n1> characters. This command */
/* means "Append <x> and then append <n0> characters starting from */
/* position P+<p> in FILE1.  Set P to P+<p>+<n0>."  Initially, P is 0 */
/* (characters in FILE1 are numbered from 0).  Numbers are */
/* represented in base 32 (see num32.h and num32.cc).  */

/* Don't bother to generate a move for any block less than */
/* MOVE_THRESHOLD long. */
const int/*waslong*/ MOVE_THRESHOLD = 6;

/* Output on cout the delta between the string */
/* T.string()[0..T.length()-1] and S1[0..N1-1], using P0 as the value */
/* of P (above), and setting it to the final value of P when done. */
/* Add OFFST to the positions of moved strings within S0. */
void diffSegment(SuffixTree& T, const byte* S1, int N1,
		 int /*waslong*/ offst, int /*waslong*/& P0)
{
    int /*waslong*/ P;
    int L;

    P = P0;
    L = N1;
    while (L > 0) {
	int/*waslong*/ start;
	int/*waslong*/ len, i;

	i = 0;
	while (true) {
	    start = P - offst; len = 0;
	    if (i >= L)
		break;
	    T.longestPrefix(S1+i, L-i, start, len);
	    if (len >= MOVE_THRESHOLD)
		break;
	    i += 1;
	}

	measureInsert (i);
	measureCopy (len, start + offst + P);

	L -= i;
	while (i > 0) {
	    S1 += 1; i -= 1;
	}
	S1 += len;
	L -= len;
	P = start + offst + len;
    }

    P0 = P;
}


main(int argc, char* argv[])
{
    ifstream F1, F2;
    byte *S1;
    byte *S2;
    int /*waslong*/ N1, N2;
    int /*waslong*/ P, offst;
    struct stat buf1, buf2;
    SuffixTree T;

    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " from to" << "\n";
	exit(1);
    }

    F1.open(argv[1]);
    if (F1.fail()) {
	cerr << "Could not open " << argv[1] << ".\n";
	exit(1);
    }

    F2.open(argv[2]);
    if (F2.fail()) {
	cerr << "Could not open " << argv[2] << ".\n";
	exit(1);
    }

    if (stat (argv[1], &buf1)) {
	cerr << "Could not stat " << argv[1] << ".\n";
	exit(1);
    }

    if (stat (argv[2], &buf2)) {
	cerr << "Could not stat " << argv[2] << ".\n";
	exit(1);
    }

    N1 = buf1.st_size;
    N2 = buf2.st_size;

    S1 = (byte*)allocate(N1);
    S2 = (byte*)allocate(N2);

    F1.read(S1, N1);
    F2.read(S2, N2);

    if (F2.fail() || F1.fail()) {
	cerr << "Read failed.\n";
	exit(1);
    }

    P = 0; offst = 0;
    T.init(S1, N1);

    diffSegment(T, S2, N2, 0, P);

    measureCopy (0, 0);

    printf ("%d\n", (cim+4)/8); /* into bytes */

    exit(0);
}
