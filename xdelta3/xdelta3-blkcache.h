/* xdelta 3 - delta compression tools and library
 * Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007,
 * 2008, 2009, 2010
 * Joshua P. MacDonald
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

typedef struct _main_blklru      main_blklru;
typedef struct _main_blklru_list main_blklru_list;

struct _main_blklru_list
{
  main_blklru_list  *next;
  main_blklru_list  *prev;
};

struct _main_blklru
{
  uint8_t          *blk;
  xoff_t            blkno;
  usize_t           size;
  main_blklru_list  link;
};

#define MAX_LRU_SIZE 32U
#define XD3_MINSRCWINSZ XD3_ALLOCSIZE

XD3_MAKELIST(main_blklru_list,main_blklru,link);

static usize_t           lru_size = 0;
static main_blklru      *lru = NULL;  /* array of lru_size elts */
static main_blklru_list  lru_list;
static main_blklru_list  lru_free;
static int               do_src_fifo = 0;  /* set to avoid lru */

static int lru_hits   = 0;
static int lru_misses = 0;
static int lru_filled = 0;

static void main_lru_reset()
{
  lru_size = 0;
  lru = NULL;
  do_src_fifo = 0;
  lru_hits   = 0;
  lru_misses = 0;
  lru_filled = 0;
}

static void main_lru_cleanup()
{
  int i;
  /* TODO(jmacd): HERE YOU ARE
   * Remove this loop, free only lru[0].blk.
   */
  for (i = 0; lru && i < lru_size; i += 1)
    {
      main_free (lru[i].blk);
    }

  main_free (lru);
  lru = NULL;

  lru_hits = 0;
  lru_misses = 0;
  lru_filled = 0;
}

/* This is called at different times for encoding and decoding.  The
 * encoder calls it immediately, the decoder delays until the
 * application header is received.  */
static int
main_set_source (xd3_stream *stream, xd3_cmd cmd,
		 main_file *sfile, xd3_source *source)
{
  int ret = 0;
  usize_t i;
  usize_t blksize;
  xoff_t source_size = 0;
  main_blklru block0;

  XD3_ASSERT (lru == NULL);
  XD3_ASSERT (stream->src == NULL);
  XD3_ASSERT (option_srcwinsz >= XD3_MINSRCWINSZ);

  main_blklru_list_init (& lru_list);
  main_blklru_list_init (& lru_free);

  if (allow_fake_source)
    {
      sfile->mode = XO_READ;
      sfile->realname = sfile->filename;
      sfile->nread = 0;
    }
  else
    {
      if ((ret = main_file_open (sfile, sfile->filename, XO_READ)))
	{
	  return ret;
	}

      /* Allow non-seekable sources from the start.  If the file
       * turns out to be externally compressed, size_known may change. */
      sfile->size_known = (main_file_stat (sfile, &source_size) == 0);
    }

  /* The API requires power-of-two blocksize, */
  blksize = xd3_pow2_roundup (max (option_srcwinsz / MAX_LRU_SIZE,
				   XD3_MINSRCWINSZ));

  /* TODO(jmacd): The organization of this code and the implementation
   * of the LRU cache could be improved.  This is too convoluted, the
   * code uses main_getblk_func() to read the first block, which may
   * trigger external-decompression, in large part so that the
   * verbose-printing and counters maintained by getblk_func are
   * consistent.  There used to be additional optimizations going on
   * here: (1) if the source size is known we would like to lower
   * option_srcwinsz, if possible, (2) avoid allocating more memory
   * than needed, and (3) also want to use a single block when
   * source_size < option_srcwinsz, this because compression is better
   * for larger blocks (especially due to the blockwise-reverse
   * insertion of checksums).
   *
   * (3) is no longer implemented.  (2) is only implemented for files
   * that are shorter than the default blocksize, and (1) is not
   * implemented.  These optimizations are not taken because the code
   * is already too complicated.
   *
   * The ideal solution may be to allocate a block of memory equal to
   * half of the option_srcwinsz.  Read as much data as possible into
   * that block.  If the entire file fits, pass one block to the
   * library for best compression.  If not, copy the 50% block into
   * smaller (option_srcwinsz / MAX_LRU_SIZE) blocks and proceed.  Too
   * much effort for too little payback. */

  memset (&block0, 0, sizeof (block0));
  block0.blkno = (xoff_t) -1;

  /* TODO(jmacd): HERE YOU ARE
   * Allocate only one block of option_srcwinsz.
   */
  /* Allocate the first block.  Even if the size is known at this
   * point, we do not lower it because decompression may happen.  */
  if ((block0.blk = (uint8_t*) main_malloc (blksize)) == NULL)
    {
      ret = ENOMEM;
      return ret;
    }

  source->blksize  = blksize;
  source->name     = sfile->filename;
  source->ioh      = sfile;
  source->curblkno = (xoff_t) -1;
  source->curblk   = NULL;

  /* We have to read the first block into the cache now, because
   * size_known can still change (due to secondary decompression).
   * Calls main_secondary_decompress_check() via
   * main_read_primary_input(). */
  lru_size = 1;
  lru = &block0;
  XD3_ASSERT (main_blklru_list_empty (& lru_free));
  XD3_ASSERT (main_blklru_list_empty (& lru_list));
  main_blklru_list_push_back (& lru_free, & lru[0]);
  /* This call is partly so that the diagnostics printed by
   * this function appear to happen normally for the first read,
   * which is special. */
  ret = main_getblk_func (stream, source, 0);
  main_blklru_list_remove (& lru[0]);
  XD3_ASSERT (main_blklru_list_empty (& lru_free));
  XD3_ASSERT (main_blklru_list_empty (& lru_list));
  lru = NULL;

  if (ret != 0)
    {
      main_free (block0.blk);

      XPR(NT "error reading source: %s: %s\n",
	  sfile->filename,
	  xd3_mainerror (ret));

      return ret;
    }

  source->onblk = block0.size;

  /* If the file is smaller than a block, size is known. */
  if (!sfile->size_known && source->onblk < blksize)
    {
      source_size = source->onblk;
      sfile->size_known = 1;
    }

  /* We update lru_size accordingly, and modify option_srcwinsz, which
   * will be passed via xd3_config. */
  if (sfile->size_known && source_size <= option_srcwinsz)
    {
      lru_size = (usize_t) ((source_size + blksize - 1) / blksize);
    }
  else
    {
      lru_size = (option_srcwinsz + blksize - 1) / blksize;
    }

  XD3_ASSERT (lru_size >= 1);
  option_srcwinsz = lru_size * blksize;

  if ((lru = (main_blklru*) main_malloc (lru_size * sizeof (main_blklru)))
      == NULL)
    {
      ret = ENOMEM;
      return ret;
    }

  lru[0].blk = block0.blk;
  lru[0].blkno = 0;
  lru[0].size = source->onblk;

  if (! sfile->size_known)
    {
      do_src_fifo = 1;
    }
  else if (! do_src_fifo)
    {
      main_blklru_list_push_back (& lru_list, & lru[0]);
    }

  for (i = 1; i < lru_size; i += 1)
    {
      lru[i].blkno = (xoff_t) -1;

      if ((lru[i].blk = (uint8_t*) main_malloc (source->blksize)) == NULL)
	{
	  ret = ENOMEM;
	  return ret;
	}

      if (! do_src_fifo)
	{
	  main_blklru_list_push_back (& lru_free, & lru[i]);
	}
    }

  if (sfile->size_known)
    {
      ret = xd3_set_source_and_size (stream, source, source_size);
    }
  else
    {
      ret = xd3_set_source (stream, source);
    }

  if (ret)
    {
      XPR(NT XD3_LIB_ERRMSG (stream, ret));
      return ret;
    }

  XD3_ASSERT (stream->src == source);
  XD3_ASSERT (source->blksize == blksize);

  if (option_verbose)
    {
      static char srcszbuf[32];
      static char srccntbuf[32];
      static char winszbuf[32];
      static char blkszbuf[32];
      static char nbufs[32];

      if (sfile->size_known)
	{
	  sprintf (srcszbuf, "source size %s [%"Q"u]",
		   main_format_bcnt (source_size, srccntbuf),
		   source_size);
	}
      else
	{
	  strcpy(srcszbuf, "source size unknown");
	}

      nbufs[0] = 0;

      if (option_verbose > 1)
	{
	  sprintf(nbufs, " #bufs %u", lru_size);
	}

      XPR(NT "source %s %s blksize %s window %s%s%s\n",
	  sfile->filename,
	  srcszbuf,
	  main_format_bcnt (blksize, blkszbuf),
	  main_format_bcnt (option_srcwinsz, winszbuf),
	  nbufs,
	  do_src_fifo ? " (FIFO)" : "");
    }

  return 0;
}

static int
main_getblk_lru (xd3_source *source, xoff_t blkno,
		 main_blklru** blrup, int *is_new)
{
  main_blklru *blru = NULL;
  usize_t i;

  (*is_new) = 0;

  if (do_src_fifo)
    {
      /* Direct lookup assumes sequential scan w/o skipping blocks. */
      int idx = blkno % lru_size;
      blru = & lru[idx];
      if (blru->blkno == blkno)
	{
	  (*blrup) = blru;
	  return 0;
	}

      if (blru->blkno != (xoff_t)-1 &&
	  blru->blkno != (xoff_t)(blkno - lru_size))
	{
	  return XD3_TOOFARBACK;
	}
    }
  else
    {
      /* Sequential search through LRU. */
      for (i = 0; i < lru_size; i += 1)
	{
	  blru = & lru[i];
	  if (blru->blkno == blkno)
	    {
	      main_blklru_list_remove (blru);
	      main_blklru_list_push_back (& lru_list, blru);
	      (*blrup) = blru;
	      return 0;
	    }
	}
    }

  if (do_src_fifo)
    {
      int idx = blkno % lru_size;
      blru = & lru[idx];
    }
  else
    {
      if (! main_blklru_list_empty (& lru_free))
	{
	  blru = main_blklru_list_pop_front (& lru_free);
	  main_blklru_list_push_back (& lru_list, blru);
	}
      else
	{
	  XD3_ASSERT (! main_blklru_list_empty (& lru_list));
	  blru = main_blklru_list_pop_front (& lru_list);
	  main_blklru_list_push_back (& lru_list, blru);
	}
    }

  lru_filled += 1;
  (*is_new) = 1;
  (*blrup) = blru;
  blru->blkno = blkno;
  return 0;
}

static int
main_read_seek_source (xd3_stream *stream,
		       xd3_source *source,
		       xoff_t      blkno) {
  xoff_t pos = blkno * source->blksize;
  main_file *sfile = (main_file*) source->ioh;
  main_blklru *blru;
  int is_new;
  usize_t nread = 0;
  int ret = 0;

  if (!sfile->seek_failed)
    {
      ret = main_file_seek (sfile, pos);

      if (ret == 0)
	{
	  sfile->source_position = pos;
	}
    }

  if (sfile->seek_failed || ret != 0)
    {
      /* For an unseekable file (or other seek error, does it
       * matter?) */
      if (sfile->source_position > pos)
	{
	  /* Could assert !IS_ENCODE(), this shouldn't happen
	   * because of do_src_fifo during encode. */
	  if (!option_quiet)
	    {
	      XPR(NT "source can't seek backwards; requested block offset "
		  "%"Q"u source position is %"Q"u\n",
		  pos, sfile->source_position);
	    }

	  sfile->seek_failed = 1;
	  stream->msg = "non-seekable source: "
	    "copy is too far back (try raising -B)";
	  return XD3_TOOFARBACK;
	}

      /* There's a chance here, that an genuine lseek error will cause
       * xdelta3 to shift into non-seekable mode, entering a degraded
       * condition.  */
      if (!sfile->seek_failed && option_verbose)
	{
	  XPR(NT "source can't seek, will use FIFO for %s\n",
	      sfile->filename);

	  if (option_verbose > 1)
	    {
	      XPR(NT "seek error at offset %"Q"u: %s\n",
		  pos, xd3_mainerror (ret));
	    }
	}

      sfile->seek_failed = 1;

      while (sfile->source_position < pos)
	{
	  xoff_t skip_blkno;
	  usize_t skip_offset;

	  xd3_blksize_div (sfile->source_position, source,
			   &skip_blkno, &skip_offset);

	  /* Read past unused data */
	  XD3_ASSERT (pos - sfile->source_position >= source->blksize);
	  XD3_ASSERT (skip_offset == 0);

	  if ((ret = main_getblk_lru (source, skip_blkno,
				      & blru, & is_new)))
	    {
	      return ret;
	    }

	  XD3_ASSERT (is_new);

	  if (option_verbose > 1)
	    {
	      XPR(NT "non-seekable source skipping %"Q"u bytes @ %"Q"u\n",
		  pos - sfile->source_position,
		  sfile->source_position);
	    }

	  if ((ret = main_read_primary_input (sfile,
					      (uint8_t*) blru->blk,
					      source->blksize,
					      & nread)))
	    {
	      return ret;
	    }

	  if (nread != source->blksize)
	    {
	      IF_DEBUG1 (DP(RINT "[getblk] short skip block nread = %u\n",
			    nread));
	      stream->msg = "non-seekable input is short";
	      return XD3_INVALID_INPUT;
	    }

	  sfile->source_position += nread;
	  blru->size = nread;

	  IF_DEBUG1 (DP(RINT "[getblk] skip blkno %"Q"u size %u\n",
			skip_blkno, blru->size));

	  XD3_ASSERT (sfile->source_position <= pos);
	}
    }

  return 0;
}

/* This is the callback for reading a block of source.  This function
 * is blocking and it implements a small LRU.
 *
 * Note that it is possible for main_input() to handle getblk requests
 * in a non-blocking manner.  If the callback is NULL then the caller
 * of xd3_*_input() must handle the XD3_GETSRCBLK return value and
 * fill the source in the same way.  See xd3_getblk for details.  To
 * see an example of non-blocking getblk, see xdelta-test.h. */
static int
main_getblk_func (xd3_stream *stream,
		  xd3_source *source,
		  xoff_t      blkno)
{
  int ret = 0;
  xoff_t pos = blkno * source->blksize;
  main_file *sfile = (main_file*) source->ioh;
  main_blklru *blru;
  int is_new;
  int did_seek = 0;
  usize_t nread = 0;

  if (allow_fake_source)
    {
      source->curblkno = blkno;
      source->onblk    = 0;
      source->curblk   = lru[0].blk;
      lru[0].size = 0;
      return 0;
    }

  if ((ret = main_getblk_lru (source, blkno, & blru, & is_new)))
    {
      return ret;
    }

  if (!is_new)
    {
      source->curblkno = blkno;
      source->onblk    = blru->size;
      source->curblk   = blru->blk;
      lru_hits++;
      return 0;
    }

  lru_misses += 1;

  if (pos != sfile->source_position)
    {
      /* Only try to seek when the position is wrong.  This means the
       * decoder will fail when the source buffer is too small, but
       * only when the input is non-seekable. */
      if ((ret = main_read_seek_source (stream, source, blkno)))
	{
	  return ret;
	}

      /* Indicates that another call to main_getblk_lru() may be
       * needed */
      did_seek = 1;
    }

  XD3_ASSERT (sfile->source_position == pos);

  if (did_seek &&
      (ret = main_getblk_lru (source, blkno, & blru, & is_new)))
    {
      return ret;
    }

  if ((ret = main_read_primary_input (sfile,
				      (uint8_t*) blru->blk,
				      source->blksize,
				      & nread)))
    {
      return ret;
    }

  /* Save the last block read, used to handle non-seekable files. */
  sfile->source_position = pos + nread;

  if (option_verbose > 3)
    {
      if (blru->blkno != (xoff_t)-1)
	{
	  if (blru->blkno != blkno)
	    {
	      XPR(NT "source block %"Q"u ejects %"Q"u (lru_hits=%u, "
		  "lru_misses=%u, lru_filled=%u)\n",
		  blkno, blru->blkno, lru_hits, lru_misses, lru_filled);
	    }
	  else
	    {
	      XPR(NT "source block %"Q"u read (lru_hits=%u, "
		  "lru_misses=%u, lru_filled=%u)\n",
		  blkno, lru_hits, lru_misses, lru_filled);
	    }
	}
      else
	{
	  XPR(NT "source block %"Q"u read (lru_hits=%u, lru_misses=%u, "
	      "lru_filled=%u)\n", blkno, lru_hits, lru_misses, lru_filled);
	}
    }

  source->curblk   = blru->blk;
  source->curblkno = blkno;
  source->onblk    = nread;
  blru->size       = nread;

  IF_DEBUG1 (DP(RINT "[main_getblk] blkno %"Q"u onblk %u pos %"Q"u "
		"srcpos %"Q"u\n",
		blkno, nread, pos, sfile->source_position));

  return 0;
}
