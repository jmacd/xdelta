/* -*- Mode: C++ -*-  */

extern "C" {
#define NOT_MAIN 1
#define REGRESSION_TEST 0
#include "../xdelta3.c"
}

#define CHECK_EQ(x,y) CHECK_OP(x,y,==)
#define CHECK_LT(x,y) CHECK_OP(x,y,<)
#define CHECK_GT(x,y) CHECK_OP(x,y,>)
#define CHECK_LE(x,y) CHECK_OP(x,y,<=)
#define CHECK_GE(x,y) CHECK_OP(x,y,>=)

#define CHECK_OP(x,y,OP) \
do {if (!((x) OP (y))) {			       \
  cerr << "Check failed: " << #x " " #OP " " #y << endl; \
  cerr << "Expected: " << x << endl; \
  cerr << "Actual: " << y << endl; \
  abort(); \
    } } while (false)

#define CHECK(x) \
  do {if (!(x)) {				       \
  cerr << "Check failed: " << #x << endl; \
  abort(); \
    } } while (false)

#include <iostream>
using std::cerr;
using std::endl;

#include <map> 
using std::map;
using std::pair;

template <typename T, typename U>
pair<T, U> make_pair(const T& t, const U& u) {
  return pair<T, U>(t, u);
}

class Constants {
public:
  static const xoff_t BLOCK_SIZE = 1 << 14;
};

using std::min;

#include "random.h"
using regtest::MTRandom;

#include "file.h"
using regtest::FileSpec;
using regtest::Block;
using regtest::BlockIterator;

#include "modify.h"
using regtest::Modify1stByte;

#include "cmp.h"
using regtest::CmpDifferentBytes;

#include "sizes.h"
using regtest::SizeIterator;
using regtest::SmallSizes;
