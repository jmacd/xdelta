%module xdelta3
%import cstring.i
%{
#include "xdelta3.h"

#undef SWIG_init
#undef SWIG_name

#define SWIG_init    initxdelta3
#define SWIG_name    "xdelta3"

%}

%cstring_input_binary(const char *input, unsigned int input_size);
%cstring_input_binary(const char *source, unsigned int source_size);
%cstring_output_withsize(char *output_buf, unsigned int avail_output);

int     xd3_encode_memory (const char *input,
			   unsigned int input_size,
			   const char *source,
			   unsigned int source_size,
			   char               *output_buf,
			   unsigned int       *OUTPUT,
			   unsigned int        avail_output,
			   int            flags);

int     xd3_decode_memory (const char         *input,
			   unsigned int        input_size,
			   const char         *source,
			   unsigned int        source_size,
			   char               *output_buf,
			   unsigned int       *OUTPUT,
			   unsigned int        avail_output,
			   int                 flags);

