/* xdelta 3 - delta compression tools and library
 * Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007. Joshua P. MacDonald
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

/******************************************************************
 SOFT string matcher
 ******************************************************************/

#if XD3_BUILD_SOFT

#define TEMPLATE      soft
#define LLOOK         stream->smatcher.large_look
#define LSTEP         stream->smatcher.large_step
#define SLOOK         stream->smatcher.small_look
#define SCHAIN        stream->smatcher.small_chain
#define SLCHAIN       stream->smatcher.small_lchain
#define MAXLAZY       stream->smatcher.max_lazy
#define LONGENOUGH    stream->smatcher.long_enough

#define SOFTCFG 1
#include "xdelta3.c"
#undef  SOFTCFG

#undef  TEMPLATE
#undef  LLOOK
#undef  SLOOK
#undef  LSTEP
#undef  SCHAIN
#undef  SLCHAIN
#undef  MAXLAZY
#undef  LONGENOUGH
#endif

#define SOFTCFG 0

/************************************************************
 FASTEST string matcher
 **********************************************************/
#if XD3_BUILD_FASTEST
#define TEMPLATE      fastest
#define LLOOK         9
#define LSTEP         26
#define SLOOK         4U
#define SCHAIN        1
#define SLCHAIN       1
#define MAXLAZY       6
#define LONGENOUGH    6

#include "xdelta3.c"

#undef  TEMPLATE
#undef  LLOOK
#undef  SLOOK
#undef  LSTEP
#undef  SCHAIN
#undef  SLCHAIN
#undef  MAXLAZY
#undef  LONGENOUGH
#endif

/************************************************************
 FASTER string matcher
 **********************************************************/
#if XD3_BUILD_FASTER
#define TEMPLATE      faster
#define LLOOK         9
#define LSTEP         15
#define SLOOK         4U
#define SCHAIN        1
#define SLCHAIN       1
#define MAXLAZY       18
#define LONGENOUGH    18

#include "xdelta3.c"

#undef  TEMPLATE
#undef  LLOOK
#undef  SLOOK
#undef  LSTEP
#undef  SCHAIN
#undef  SLCHAIN
#undef  MAXLAZY
#undef  LONGENOUGH
#endif

/******************************************************
 FAST string matcher
 ********************************************************/
#if XD3_BUILD_FAST
#define TEMPLATE      fast
#define LLOOK         9
#define LSTEP         8
#define SLOOK         4U
#define SCHAIN        4
#define SLCHAIN       1
#define MAXLAZY       18
#define LONGENOUGH    35

#include "xdelta3.c"

#undef  TEMPLATE
#undef  LLOOK
#undef  SLOOK
#undef  LSTEP
#undef  SCHAIN
#undef  SLCHAIN
#undef  MAXLAZY
#undef  LONGENOUGH
#endif

/**************************************************
 SLOW string matcher
 **************************************************************/
#if XD3_BUILD_SLOW
#define TEMPLATE      slow
#define LLOOK         9
#define LSTEP         2
#define SLOOK         4U
#define SCHAIN        44
#define SLCHAIN       13
#define MAXLAZY       90
#define LONGENOUGH    70

#include "xdelta3.c"

#undef  TEMPLATE
#undef  LLOOK
#undef  SLOOK
#undef  LSTEP
#undef  SCHAIN
#undef  SLCHAIN
#undef  MAXLAZY
#undef  LONGENOUGH
#endif

/********************************************************
 DEFAULT string matcher
 ************************************************************/
#if XD3_BUILD_DEFAULT
#define TEMPLATE      default
#define LLOOK         9
#define LSTEP         3
#define SLOOK         4U
#define SCHAIN        8
#define SLCHAIN       2
#define MAXLAZY       36
#define LONGENOUGH    70

#include "xdelta3.c"

#undef  TEMPLATE
#undef  LLOOK
#undef  SLOOK
#undef  LSTEP
#undef  SCHAIN
#undef  SLCHAIN
#undef  MAXLAZY
#undef  LONGENOUGH
#endif
