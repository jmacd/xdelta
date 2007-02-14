/* Copyright (C) 2007 Josh MacDonald */

#include <stdio.h>

#define SPACE_MAX 32768
#define PAGE_SIZE 4096
#define OUTPUT_MAX 1024
#define IOPT_SIZE 1024
#define XD3_ALLOCSIZE 256

// typedef void*  (xd3_alloc_func)    (void       *opaque,
// 				    usize_t      items,
// 				    usize_t      size);
// typedef void   (xd3_free_func)     (void       *opaque,
// 				    void       *address);

#include "xdelta3.h"
#include "xdelta3.c"

typedef struct _context {
  uint8_t *buffer;
  int allocated;
} context_t;

void*
process_alloc (void* opaque, usize_t items, usize_t size)
{
  context_t *ctx = (context_t*) opaque;
  usize_t t = items * size;
  void *ret;

  if (ctx->allocated + t > SPACE_MAX)
    {
      return NULL;
    }

  ret = ctx->buffer + ctx->allocated;
  ctx->allocated += t;
  return ret;
}

int
process_page (int            is_encode,
	      int          (*func) (xd3_stream *),
	      const uint8_t *input,
	      usize_t        input_size,
	      const uint8_t *source,
	      uint8_t       *output,
	      usize_t       *output_size,
	      usize_t        output_size_max,
	      int            flags) {
  xd3_stream *stream;
  xd3_config *config;
  xd3_source *src;
  context_t *ctx = calloc(OUTPUT_MAX, 1);
  int ret;

  ctx->buffer = ((char*)ctx) + sizeof(*ctx);
  ctx->allocated = sizeof(*ctx);

  stream = process_alloc (ctx, 1, sizeof(*stream));
  config = process_alloc (ctx, 1, sizeof(*config));
  src = process_alloc (ctx, 1, sizeof(*src));

  config->flags = flags;
  config->winsize = PAGE_SIZE;
  config->sprevsz = PAGE_SIZE;
  config->srcwin_maxsz = PAGE_SIZE;
  config->iopt_size = IOPT_SIZE;
  config->alloc = &process_alloc;

  src->size = PAGE_SIZE;
  src->blksize = PAGE_SIZE;
  src->onblk = PAGE_SIZE;
  src->curblk = source;
  src->curblkno = 0;

  if ((ret = xd3_config_stream (stream, config)) != 0 ||
      (ret = xd3_set_source (stream, src)) != 0 ||
      (ret = xd3_process_stream (is_encode,
				 stream,
				 func, 1,
				 input, PAGE_SIZE,
				 output, output_size,
				 output_size_max)) != 0)
    {
      // (void) 0;
    }

  xd3_free_stream (stream);
  return ret;
}

int test(int stride, int encode_flags)
{
  uint8_t frompg[PAGE_SIZE];
  uint8_t topg[PAGE_SIZE];
  uint8_t output[OUTPUT_MAX];
  uint8_t reout[OUTPUT_MAX];
  usize_t output_size;
  usize_t re_size;
  int i, j, ret;

  for (i = 0; i < PAGE_SIZE; i++)
    {
      topg[i] = frompg[i] = lrand48();
    }

  // change 1 byte every stride
  if (stride > 0)
    {
      for (j = stride; j <= PAGE_SIZE; j += stride)
	{
	  topg[j - 1] ^= 0xff;
	}
    }

  if ((ret = process_page (1, xd3_encode_input,
			   topg, PAGE_SIZE,
			   frompg, output,
			   &output_size, OUTPUT_MAX,
			   encode_flags)) != 0)
    {
      return ret;
    }

  if ((ret = process_page (1, xd3_decode_input,
			   output, output_size,
			   frompg, reout,
			   &re_size, PAGE_SIZE,
			   0)) != 0)
    {
      return ret;
    }

  if (output_size > OUTPUT_MAX || re_size != PAGE_SIZE)
    {
      printf ("internal error\n");
      return -1;
    }

  printf("stride %d flags 0x%x size %u ", stride, encode_flags, output_size);
  printf("%s\n", (ret == 0) ? "OK" : "FAIL");

  return 0;
}

int main()
{
  int stride;
  int level;
  int ret;

  for (level = 1; level < 10; level = (level == 1 ? 3 : level + 3))
    {
      int lflag = level << XD3_COMPLEVEL_SHIFT;
      for (stride = 0; stride <= PAGE_SIZE; stride += PAGE_SIZE / 64)
	{
	  if ((ret = test(stride, lflag)) ||
	      (ret = test(stride, lflag | XD3_ADLER32)) ||
	      (ret = test(stride, lflag | XD3_SEC_DJW)))
	    {
	      return ret;
	    }
	}
    }

  return 0;
}
