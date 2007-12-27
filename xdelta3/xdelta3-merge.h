/* xdelta 3 - delta compression tools and library
 * Copyright (C) 2007.  Joshua P. MacDonald
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

#ifndef _XDELTA3_MERGE_H_
#define _XDELTA3_MERGE_H_

static int
xd3_realloc_buffer (xd3_stream *stream,
                    usize_t current_units,
                    usize_t unit_size,
                    usize_t new_units,
                    usize_t *alloc_size,
                    void **alloc_ptr)
{
  usize_t needed;
  usize_t new_alloc;
  usize_t cur_size;
  uint8_t *new_buf;

  needed = (current_units + new_units) * unit_size;

  if (needed <= *alloc_size)
    {
      return 0;
    }

  cur_size = current_units * unit_size;
  new_alloc = xd3_round_blksize (needed * 2, XD3_ALLOCSIZE);

  if ((new_buf = xd3_alloc (stream, new_alloc, 1)) == NULL)
    {
      return ENOMEM;
    }

  if (cur_size != 0)
    {
      memcpy (new_buf, *alloc_ptr, cur_size);
    }

  if (*alloc_ptr != NULL)
    {
      xd3_free (stream, *alloc_ptr);
    }

  *alloc_size = new_alloc;
  *alloc_ptr = new_buf;

  return 0;
}

static int
xd3_whole_append_inst (xd3_stream *stream,
                       xd3_hinst *inst)
{
  int ret;
  xd3_winst *winst;

  if ((ret = xd3_realloc_buffer (stream,
                                 stream->whole_target_instlen,
                                 sizeof (xd3_winst),
                                 1,
                                 & stream->whole_target_inst_alloc,
                                 (void**) & stream->whole_target_inst)))
    {
      return ret;
    }

  if ((inst->type <= XD3_ADD) &&
      (ret = xd3_realloc_buffer (stream,
                                 stream->whole_target_addslen,
                                 1,
                                 (inst->type == XD3_RUN ? 1 : inst->size),
                                 & stream->whole_target_adds_alloc,
                                 (void**) & stream->whole_target_adds)))
    {
      return ret;
    }

  winst = &stream->whole_target_inst[stream->whole_target_instlen++];
  winst->type = inst->type;
  winst->mode = SRCORTGT (stream->dec_win_ind);
  winst->size = inst->size;

  switch (inst->type)
    {
    case XD3_RUN:
      winst->addr = stream->whole_target_addslen;
      stream->whole_target_adds[stream->whole_target_addslen++] =
        *stream->data_sect.buf++;
      break;

    case XD3_ADD:
      winst->addr = stream->whole_target_addslen;
      memcpy (stream->whole_target_adds + stream->whole_target_addslen,
              stream->data_sect.buf,
              inst->size);
      stream->data_sect.buf += inst->size;
      stream->whole_target_addslen += inst->size;
      break;

    default:
      winst->addr = stream->dec_cpyoff + inst->addr;
      break;
    }

  return 0;
}

int
xd3_whole_append_window (xd3_stream *stream)
{
  int ret;

  while (stream->inst_sect.buf < stream->inst_sect.buf_max)
    {
      if ((ret = xd3_decode_instruction (stream)))
	{
	  return ret;
	}

      if ((stream->dec_current1.type != XD3_NOOP) &&
          (ret = xd3_whole_append_inst (stream,
                                        & stream->dec_current1)))
	{
          return ret;
	}

      if ((stream->dec_current2.type != XD3_NOOP) &&
          (ret = xd3_whole_append_inst (stream,
                                        & stream->dec_current2)))
	{
          return ret;
	}
    }

  return 0;
}

#endif
