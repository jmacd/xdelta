/* Copyright (C) 2007 Josh MacDonald */

#define NOT_MAIN 1

#include "xdelta3.h"
#include "xdelta3.c"

static int read_whole_file(const char *name,
			   uint8_t **buf_ptr,
			   size_t *buf_len) {
  main_file file;
  int ret;
  xoff_t len;
  usize_t nread;
  main_file_init(&file);
  file.filename = name;
  ret = main_file_open(&file, name, XO_READ);
  if (ret != 0) {
    fprintf(stderr, "open failed\n");
    goto exit;
  }
  ret = main_file_stat(&file, &len);
  if (ret != 0) {
    fprintf(stderr, "stat failed\n");
    goto exit;
  }
  
  (*buf_len) = (size_t)len;
  (*buf_ptr) = (uint8_t*) main_malloc(*buf_len);
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

