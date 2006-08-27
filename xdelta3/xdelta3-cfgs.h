/* xdelta 3 - delta compression tools and library
 * Copyright (C) 2001 and onward.  Joshua P. MacDonald
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/******************************************************************************************
 SOFT string matcher
 ******************************************************************************************/

#if XD3_BUILD_SOFT

#define TEMPLATE      soft
#define LLOOK         stream->large_look
#define LSTEP         stream->large_step
#define SLOOK         stream->small_look
#define SCHAIN        stream->small_chain
#define SLCHAIN       stream->small_lchain
#define SSMATCH       stream->ssmatch
#define TRYLAZY       stream->try_lazy
#define MAXLAZY       stream->max_lazy
#define LONGENOUGH    stream->long_enough
#define PROMOTE       stream->promote

#define SOFTCFG 1
#include "xdelta3.c"
#undef  SOFTCFG

#undef  TEMPLATE
#undef  LLOOK
#undef  SLOOK
#undef  LSTEP
#undef  SCHAIN
#undef  SLCHAIN
#undef  SSMATCH
#undef  TRYLAZY
#undef  MAXLAZY
#undef  LONGENOUGH
#undef  PROMOTE
#endif

#define SOFTCFG 0

/******************************************************************************************
 FAST string matcher
 ******************************************************************************************/
#if XD3_BUILD_FAST
#define TEMPLATE      fast
#define LLOOK         32
#define LSTEP         32
#define SLOOK         4

#define SCHAIN        2 // For testcase/3, this produces miserable performance
#define SLCHAIN       2 // with these values != 1 and large input window size

#define SSMATCH       1
#define TRYLAZY       0
#define MAXLAZY       0
#define LONGENOUGH    64
#define PROMOTE       0

#include "xdelta3.c"

#undef  TEMPLATE
#undef  LLOOK
#undef  SLOOK
#undef  LSTEP
#undef  SCHAIN
#undef  SLCHAIN
#undef  SSMATCH
#undef  TRYLAZY
#undef  MAXLAZY
#undef  LONGENOUGH
#undef  PROMOTE
#endif

/******************************************************************************************
 SLOW string matcher
 ******************************************************************************************/
#if XD3_BUILD_SLOW
#define TEMPLATE      slow
#define LLOOK         64
#define LSTEP         64 // TODO
#define SLOOK         4
#define SCHAIN        128
#define SLCHAIN       16
#define SSMATCH       0
#define TRYLAZY       1
#define MAXLAZY       8
#define LONGENOUGH    128
#define PROMOTE       0

#include "xdelta3.c"

#undef  TEMPLATE
#undef  LLOOK
#undef  SLOOK
#undef  LSTEP
#undef  SCHAIN
#undef  SLCHAIN
#undef  SSMATCH
#undef  TRYLAZY
#undef  MAXLAZY
#undef  LONGENOUGH
#undef  PROMOTE
#endif
