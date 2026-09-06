/* -*- Mode: C;-*-
 *
 * This file is part of XDelta - A binary delta generator.
 *
 * Copyright (C) 1997, 1998, 1999  Josh MacDonald
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
 * $Id: xdapply.c 1.1 Thu, 01 Apr 1999 23:29:11 -0800 jmacd $
 */

#include <string.h>
#include <stdlib.h>

#include "xdelta.h"
#include "xdeltapriv.h"

/* This code used to be more general, but implemented a very
 * inefficient algorithm.  It is sufficient (and efficient) for the
 * special case below, though, so I'm leaving it. */
static gboolean
xdp_copy_delta_region    (XdeltaControl     *cont,
			  XdeltaOutStream   *output_stream)
{
  gint i, l = cont->inst_len;
  guint save_written = 0;

  for (i = 0; i < l; i += 1)
    {
      const XdeltaInstruction *inst = cont->inst + i;
      XdeltaSourceInfo* info;

      if (inst->index >= cont->source_info_len)
	{
	  xd_generate_int_event (EC_XdOutOfRangeSourceIndex, inst->index);
	  return FALSE;
	}

      info = cont->source_info[inst->index];

      if (! handle_copy (info->in, output_stream, inst->offset, inst->length))
	return FALSE;

      save_written += inst->length;
    }

  return TRUE;
}

gboolean
xdp_apply_delta (XdeltaControl     *cont,
		 XdeltaOutStream   *res)
{
  if (! xdp_copy_delta_region (cont, res))
    return FALSE;

  if (! handle_close (res, 0))
    return FALSE;

  if (! check_stream_integrity (res, cont->to_md5, cont->to_len))
    {
      int i;

      /* to better report errors, check if the inputs were invalid now
       */
      for (i = 0; i < cont->source_info_len; i += 1)
	{
	  check_stream_integrity (cont->source_info[i]->in,
				  cont->source_info[i]->md5,
				  cont->source_info[i]->len);
	}

      return FALSE;
    }

  return TRUE;
}
