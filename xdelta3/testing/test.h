// -*- Mode: C++ -*-

extern "C" {
#define NOT_MAIN 1
#define REGRESSION_TEST 0
#include "../xdelta3.c"
}

#define CHECK_EQ(x,y) CHECK_OP(x,y,==)
#define CHECK_NE(x,y) CHECK_OP(x,y,!=)
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

//#define VLOG(n) if ((n) >= debug_level) cerr
//static int debug_level;

#include <iostream>
using std::cerr;
using std::endl;

#include <map> 
using std::map;
using std::pair;

#include <list>
using std::list;

template <typename T, typename U>
pair<T, U> make_pair(const T& t, const U& u) {
  return pair<T, U>(t, u);
}

class Constants {
public:
  static const xoff_t BLOCK_SIZE = 1 << 15;
};

using std::min;

#include "segment.h"

#include "random.h"
using regtest::MTRandom;

#include "modify.h"
using regtest::Mutator;
using regtest::ChangeList;
using regtest::Change;
using regtest::ChangeListMutator;
using regtest::Modify1stByte;

#include "file.h"
using regtest::FileSpec;
using regtest::Block;
using regtest::BlockIterator;

#include "cmp.h"
using regtest::CmpDifferentBytes;

#include "sizes.h"
using regtest::SizeIterator;
using regtest::SmallSizes;
