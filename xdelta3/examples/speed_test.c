/* Copyright (C) 2007 Josh MacDonald */

#define NOT_MAIN 1

#include "xdelta3.h"
#include "xdelta3.c"

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

int read_whole_file(const char *name,
		    uint8_t **buf_ptr,
		    size_t *buf_len) {
  main_file file;
  int ret;
  xoff_t len;
  size_t nread;
  main_file_init(&file);
  file.filename = name;
  ret = main_file_open(&file, name, XO_READ);
  if (ret != 0) {
    goto exit;
  }
  ret = main_file_stat(&file, &len, 1);
  if (ret != 0) {
    goto exit;
  }
  
  (*buf_len) = (size_t)len;
  (*buf_ptr) = main_malloc(*buf_len);
  ret = main_file_read(&file, *buf_ptr, *buf_len, &nread,
		       "read failed");
  if (ret == 0 && *buf_len == nread) {
    ret = 0;
  } else {
    fprintf(stderr, "invalid read\n");
    ret = XD3_INTERNAL;
  }
 exit:
  main_file_cleanup(&file);
  return ret;
}

int main(int argc, char **argv) {
  int repeat;
  char *from, *to;
  uint8_t *from_buf = NULL, *to_buf = NULL, *delta_buf = NULL;
  size_t from_len, to_len, delta_alloc, delta_size = 0;
  long start, finish;
  int i, ret;
  int flags = XD3_COMPLEVEL_1;

  if (argc != 4) {
    fprintf(stderr, "usage: speed_test COUNT FROM TO\n");
    return 1;
  }

  repeat = atoi(argv[1]);
  from = argv[2];
  to = argv[3];

  if ((ret = read_whole_file(from, &from_buf, &from_len)) ||
      (ret = read_whole_file(to, &to_buf, &to_len))) {
    fprintf(stderr, "read_whole_file error\n");
    goto exit;
  }

  delta_alloc = to_len * 11 / 10;
  delta_buf = main_malloc(delta_alloc);

  start = get_millisecs_now();

  for (i = 0; i < repeat; ++i) {
    delta_size = bench_speed(from_buf, from_len, to_buf, to_len, delta_buf, delta_alloc, flags);
  }

  finish = get_millisecs_now();

  fprintf(stderr,
	  "STAT: encode %3ld ms from %s to %s repeat %d %dbit delta %d\n",
	  (finish - start) / repeat, from, to, repeat, sizeof (xoff_t) * 8, delta_size);

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
