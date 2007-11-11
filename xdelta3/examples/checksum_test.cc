/* Copyright (C) 2007 Josh MacDonald */

extern "C" {
#include "test.h"
#include <assert.h>
}

// template <typename T, int Cklen>
// struct cksum_params {
//   typedef T cksum_type;
//   enum { cklen = Cklen };
// };

template <int Cklen>
struct rabin_karp {
  enum { cklen = Cklen, };
};

template<typename T>
struct test_result {

  int n_data;

  test_result()
    : n_data(0) {
  }

  void print() {
    fprintf(stderr, "cklen %u: %u results\n", T::cklen, n_data);
  }

  void add(uint8_t* ptr) {
    n_data++;
  }
};

typedef rabin_karp<4> small_cksum;
typedef rabin_karp<9> large_cksum;

template<typename T>
void test(uint8_t* buf, usize_t buf_len) {
  test_result<T> result;

  for (usize_t i = 0; i < buf_len - T::cklen; i++) {
    result.add(buf + i);
  }

  result.print();
}

int main(int argc, char** argv) {
  int i;
  uint8_t *buf = NULL;
  usize_t buf_len = 0;
  int ret;

  if (argc <= 1) {
    fprintf(stderr, "usage: %s file ...", argv[0]);
    return 1;
  }

  for (i = 1; i < argc; i++) {
    if ((ret = read_whole_file(argv[i],
			       & buf,
			       & buf_len))) {
      return 1;
    }

    fprintf(stderr, "file %s is %u bytes\n", argv[i], buf_len);

    test<small_cksum>(buf, buf_len);
    test<large_cksum>(buf, buf_len);

    free(buf);
    buf = NULL;
  }

  return 0;      
}
