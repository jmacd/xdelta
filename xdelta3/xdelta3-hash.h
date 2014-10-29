/* xdelta 3 - delta compression tools and library
 * Copyright (C) 2001, 2003, 2004, 2005, 2006, 2007, 2011, 2012, 2014.
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

#ifndef _XDELTA3_HASH_H_
#define _XDELTA3_HASH_H_

#include "xdelta3-internal.h"

#if XD3_DEBUG
#define SMALL_HASH_DEBUG1(s,inp)                                  \
  uint32_t debug_state;                                           \
  uint32_t debug_hval = xd3_checksum_hash (& (s)->small_hash,     \
              xd3_scksum (&debug_state, (inp), (s)->smatcher.small_look))
#define SMALL_HASH_DEBUG2(s,inp)                                  \
  XD3_ASSERT (debug_hval == xd3_checksum_hash (& (s)->small_hash, \
              xd3_scksum (&debug_state, (inp), (s)->smatcher.small_look)))
#else
#define SMALL_HASH_DEBUG1(s,inp)
#define SMALL_HASH_DEBUG2(s,inp)
#endif /* XD3_DEBUG */

#if UNALIGNED_OK
#define UNALIGNED_READ32(dest,src) (*(dest)) = (*(uint32_t*)(src))
#else
#define UNALIGNED_READ32(dest,src) memcpy((dest), (src), 4);
#endif

/* These are good hash multipliers for 32-bit and 64-bit LCGs: see
 * "linear congruential generators of different sizes and good lattice
 * structure" */
#define xd3_hash_multiplier32 1597334677U
#define xd3_hash_multiplier64 1181783497276652981ULL

/* TODO: small cksum is hard-coded for 4 bytes (i.e., "look" is unused) */
static inline uint32_t
xd3_scksum (uint32_t *state,
            const uint8_t *base,
            const usize_t look)
{
  UNALIGNED_READ32(state, base);
  return (*state) * xd3_hash_multiplier32;
}
static inline uint32_t
xd3_small_cksum_update (uint32_t *state,
			const uint8_t *base,
			usize_t look)
{
  UNALIGNED_READ32(state, base+1);
  return (*state) * xd3_hash_multiplier32;
}

#if XD3_ENCODER
inline usize_t
xd3_checksum_hash (const xd3_hash_cfg *cfg, const usize_t cksum)
{
  return (cksum >> cfg->shift) ^ (cksum & cfg->mask);
}

#if SIZEOF_USIZE_T == 4
inline uint32_t
xd3_large32_cksum (xd3_hash_cfg *cfg, const uint8_t *base, const usize_t look)
{
  uint32_t h = 0;
  for (usize_t i = 0; i < look; i++) {
    h += base[i] * cfg->powers[i];
  }
  return h;
}

inline uint32_t
xd3_large32_cksum_update (xd3_hash_cfg *cfg, const uint32_t cksum,
			  const uint8_t *base, const usize_t look)
{
  return xd3_hash_multiplier32 * cksum - cfg->multiplier * base[0] + base[look];
}
#endif

#if SIZEOF_USIZE_T == 8
inline uint64_t
xd3_large64_cksum (xd3_hash_cfg *cfg, const uint8_t *base, const usize_t look)
{
  uint64_t h = 0;
  for (usize_t i = 0; i < look; i++) {
    h += base[i] * cfg->powers[i];
  }
  return h;
}

inline uint64_t
xd3_large64_cksum_update (xd3_hash_cfg *cfg, const uint64_t cksum,
			  const uint8_t *base, const usize_t look)
{
  return xd3_hash_multiplier64 * cksum - cfg->multiplier * base[0] + base[look];
}
#endif

static usize_t
xd3_size_hashtable_bits (usize_t slots)
{
  usize_t bits = (SIZEOF_USIZE_T * 8) - 1;
  usize_t i;

  for (i = 3; i <= bits; i += 1)
    {
      if (slots < (1U << i))
	{
	  /* Note: this is the compaction=1 setting measured in
	   * checksum_test */
	  bits = i - 1;
	  break;
	}
    }

  return bits;
}

int
xd3_size_hashtable (xd3_stream   *stream,
		    usize_t       slots,
		    usize_t       look,
		    xd3_hash_cfg *cfg)
{
  usize_t bits = xd3_size_hashtable_bits (slots);

  cfg->size  = (1U << bits);
  cfg->mask  = (cfg->size - 1);
  cfg->shift = (SIZEOF_USIZE_T * 8) - bits;
  cfg->look  = look;

  if ((cfg->powers = 
       (usize_t*) xd3_alloc0 (stream, look, sizeof (usize_t))) == NULL)
    {
      return ENOMEM;
    }

  cfg->powers[look-1] = 1;
  for (int i = look-2; i >= 0; i--)
    {
      cfg->powers[i] = cfg->powers[i+1] * xd3_hash_multiplier;
    }
  cfg->multiplier = cfg->powers[0] * xd3_hash_multiplier;

  return 0;
}

#endif /* XD3_ENCODER */
#endif /* _XDELTA3_HASH_H_ */
