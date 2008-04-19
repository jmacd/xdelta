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

int xd3_merge_inputs (xd3_stream *stream, 
		      xd3_whole_state *source,
		      xd3_whole_state *input);

static int
xd3_whole_state_init (xd3_stream *stream)
{
  XD3_ASSERT (stream->whole_target.adds == NULL);
  XD3_ASSERT (stream->whole_target.inst == NULL);

  stream->whole_target.adds_alloc = XD3_ALLOCSIZE;
  stream->whole_target.inst_alloc = XD3_ALLOCSIZE / sizeof (xd3_winst);

  if ((stream->whole_target.adds = (uint8_t*) 
       xd3_alloc (stream, XD3_ALLOCSIZE, 1)) == NULL ||
      (stream->whole_target.inst = (xd3_winst*) 
       xd3_alloc (stream, XD3_ALLOCSIZE, sizeof(xd3_winst))) == NULL)
    {
      return ENOMEM;
    }
  return 0;
}

static void
xd3_swap_whole_state (xd3_whole_state *a, 
		      xd3_whole_state *b)
{
  xd3_whole_state tmp;
  XD3_ASSERT (a->inst != NULL && a->adds != NULL);
  XD3_ASSERT (b->inst != NULL && b->adds != NULL);
  memcpy (&tmp, a, sizeof (xd3_whole_state));
  memcpy (a, b, sizeof (xd3_whole_state));
  memcpy (b, &tmp, sizeof (xd3_whole_state));
}

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
                                 stream->whole_target.instlen,
                                 sizeof (xd3_winst),
                                 1,
                                 & stream->whole_target.inst_alloc,
                                 (void**) & stream->whole_target.inst)))
    {
      return ret;
    }

  if ((inst->type <= XD3_ADD) &&
      (ret = xd3_realloc_buffer (stream,
                                 stream->whole_target.addslen,
                                 1,
                                 (inst->type == XD3_RUN ? 1 : inst->size),
                                 & stream->whole_target.adds_alloc,
                                 (void**) & stream->whole_target.adds)))
    {
      return ret;
    }

  winst = &stream->whole_target.inst[stream->whole_target.instlen++];
  winst->type = inst->type;
  winst->mode = 0;
  winst->size = inst->size;

  switch (inst->type)
    {
    case XD3_RUN:
      winst->addr = stream->whole_target.addslen;
      stream->whole_target.adds[stream->whole_target.addslen++] =
        *stream->data_sect.buf++;
      break;

    case XD3_ADD:
      winst->addr = stream->whole_target.addslen;
      memcpy (stream->whole_target.adds + stream->whole_target.addslen,
              stream->data_sect.buf,
              inst->size);
      stream->data_sect.buf += inst->size;
      stream->whole_target.addslen += inst->size;
      break;

    default:
      if (inst->addr < stream->dec_cpylen)
	{
	  winst->mode = SRCORTGT (stream->dec_win_ind);
	  winst->addr = stream->dec_cpyoff + inst->addr;
	}
      else
	{
	  winst->addr = stream->total_out + inst->addr - stream->dec_cpylen;
	}
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

/* xd3_merge_input_output applies *source to *stream, returns the
 * result in stream. */
int xd3_merge_input_output (xd3_stream *stream,
			    xd3_whole_state *source)
{
  int ret;
  xd3_stream tmp_stream;
  memset (& tmp_stream, 0, sizeof (tmp_stream));
  if ((ret = xd3_config_stream (& tmp_stream, NULL)) ||
      (ret = xd3_whole_state_init (& tmp_stream)) ||
      (ret = xd3_merge_inputs (& tmp_stream, 
			       source,
			       & stream->whole_target)))
    {
      XPR(NT XD3_LIB_ERRMSG (&tmp_stream, ret));
      return ret;
    }

  /* the output is in tmp_stream.whole_state, swap into input */
  xd3_swap_whole_state (& stream->whole_target,
			& tmp_stream.whole_target);
  /* total allocation counts are preserved */
  xd3_free_stream (& tmp_stream);
  return 0;
}

/* xd3_merge_inputs() applies *input to *source, returns its result in
 * stream.  In certain cases, input == stream->whole_target. */
int xd3_merge_inputs (xd3_stream *stream, 
		      xd3_whole_state *source,
		      xd3_whole_state *input)
{


  xd3_swap_whole_state (&stream->whole_target, input);
  
  return 0;
}

#endif
