// -*- Mode: C++ -*-

extern "C" {
#include "../xdelta3.h"
#include "../xdelta3-internal.h"
}

#include <unistd.h>
#include <math.h>
#include <string>

#define CHECK_EQ(x,y) CHECK_OP(x,y,==)
#define CHECK_NE(x,y) CHECK_OP(x,y,!=)
#define CHECK_LT(x,y) CHECK_OP(x,y,<)
#define CHECK_GT(x,y) CHECK_OP(x,y,>)
#define CHECK_LE(x,y) CHECK_OP(x,y,<=)
#define CHECK_GE(x,y) CHECK_OP(x,y,>=)

#define CHECK_OP(x,y,OP) \
  do { \
    typeof(x) _x(x); \
    typeof(x) _y(y); \
    if (!(_x OP _y)) { \
      cerr << __FILE__ << ":" << __LINE__ << " Check failed: " << #x " " #OP " " #y << endl; \
      cerr << __FILE__ << ":" << __LINE__ << " Expected: " << _x << endl; \
      cerr << __FILE__ << ":" << __LINE__ << " Actual: " << _y << endl; \
    abort(); \
    } } while (false)
#undef CHECK
#define CHECK(x) \
  do {if (!(x)) {				       \
  cerr << __FILE__ << ":" << __LINE__ << " Check failed: " << #x << endl; \
  abort(); \
    } } while (false)

#define DCHECK(x)

using std::string;

#include <vector>
using std::vector;

inline string CommandToString(const vector<const char*> &v) {
  string s(v[0]);
  for (size_t i = 1; i < v.size() && v[i] != NULL; i++) {
    s.append(" ");
    s.append(v[i]);
  }
  return s;
}

#include <iostream>
using std::cerr;
using std::endl;
using std::ostream;

#include <map> 
using std::map;
using std::pair;

#include <list>
using std::list;

template <typename T, typename U>
pair<T, U> make_pair(const T& t, const U& u) {
  return pair<T, U>(t, u);
}

using std::min;
using std::max;


