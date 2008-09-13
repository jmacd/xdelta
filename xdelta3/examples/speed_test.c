/* Copyright (C) 2007 Josh MacDonald */

#include "test.h"

usize_t bench_speed(const uint8_t *from_buf, const size_t from_len,
		 const uint8_t *to_buf, const size_t to_len,
		 uint8_t *delta_buf, const size_t delta_alloc,
		 int flags) {
  usize_t delta_size;
  int ret = xd3_encode_memory(to_buf, to_len, from_buf, from_len,
			      delta_buf, &delta_size, delta_alloc, flags);
  if (ret != 0) {
    fprintf(stderr, "encode failure: %d: %s\n", ret, xd3_strerror(ret));
    abort();
  }
  return delta_size;
}

int main(int argc, char **argv) {
  int repeat, level;
  char *from, *to;
  uint8_t *from_buf = NULL, *to_buf = NULL, *delta_buf = NULL;
  size_t from_len = 0, to_len, delta_alloc, delta_size = 0;
  long start, finish;
  int i, ret;
  int flags;

  if (argc != 5) {
    fprintf(stderr, "usage: speed_test LEVEL COUNT FROM TO\n");
    return 1;
  }

  level = atoi(argv[1]);
  repeat = atoi(argv[2]);
  from = argv[3];
  to = argv[4];
  flags = (level << XD3_COMPLEVEL_SHIFT) & XD3_COMPLEVEL_MASK;

  if ((strcmp(from, "null") != 0 &&
       (ret = read_whole_file(from, &from_buf, &from_len))) ||
      (ret = read_whole_file(to, &to_buf, &to_len))) {
    fprintf(stderr, "read_whole_file error\n");
    goto exit;
  }

  delta_alloc = to_len * 11 / 10;
  delta_buf = main_malloc(delta_alloc);

  start = get_millisecs_now();

  for (i = 0; i < repeat; ++i) {
    delta_size = bench_speed(from_buf, from_len,
			     to_buf, to_len, delta_buf, delta_alloc, flags);
  }

  finish = get_millisecs_now();

  fprintf(stderr,
	  "STAT: encode %3.2f ms from %s to %s repeat %d %zdbit delta %zd\n",
	  (double)(finish - start) / repeat, from, to, repeat, sizeof (xoff_t) * 8, delta_size);

  ret = 0;

  if (0) {
  exit:
    ret = 1;
  }
    
  main_free(to_buf);
  main_free(from_buf);
  main_free(delta_buf);
  return ret;
}
