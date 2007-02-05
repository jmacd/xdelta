%module xdelta3
%{
#include "xdelta3.h"

DL_EXPORT(void) initxdelta3 (void)
{
  SWIG_init();
}

%}

int     xd3_encode_memory (const uint8_t *input,
			   usize_t        input_size,
			   const uint8_t *source,
			   usize_t        source_size,
			   uint8_t       *output,
			   usize_t       *output_size,
			   usize_t        avail_output,
			   int            flags);

int     xd3_decode_memory (const uint8_t *input,
			   usize_t        input_size,
			   const uint8_t *source,
			   usize_t        source_size,
			   uint8_t       *output,
			   usize_t       *output_size,
			   usize_t        avail_output,
			   int            flags);

