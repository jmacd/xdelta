/* Copyright (C) 2007 Josh MacDonald */

extern "C" {
#include "test.h"
}

#include <list>
#include <vector>
#include <map>
#include <algorithm>

using std::list;
using std::map;
using std::vector;

// MLCG parameters
// a, a*
uint32_t good_32bit_values[] = {
    1597334677U, // ...
    741103597U, 887987685U,
};

// a, a*
uint64_t good_64bit_values[] = {
    1181783497276652981ULL, 4292484099903637661ULL,
    7664345821815920749ULL, // ...
};

struct true_type { };
struct false_type { };

template <typename Word>
int bitsof();

template<>
int bitsof<uint32_t>() {
    return 32;
}

template<>
int bitsof<uint64_t>() {
    return 64;
}

struct plain {
    int operator()(const uint8_t &c) {
	return c;
    }
};

template <typename Word>
struct hhash {  // take "h" of the high-bits as a hash value for this
		// checksum, which are the most "distant" in terms of the
		// spectral test for the rabin_karp MLCG.  For short windows,
		// the high bits aren't enough, XOR "mask" worth of these in.
    Word operator()(const Word& t, const int &h, const int &mask) {
	return (t >> h) ^ (t & mask);
    }
};

template <typename Word>
Word good_word();

template<>
uint32_t good_word<uint32_t>() {
    return good_32bit_values[0];
}

template<>
uint64_t good_word<uint64_t>() {
    return good_64bit_values[0];
}

// CLASSES

#define SELF Word, CksumSize, CksumSkip, Permute, Hash, Compaction
#define MEMBER template <typename Word, \
			 int CksumSize, \
			 int CksumSkip, \
			 typename Permute, \
			 typename Hash, \
                         int Compaction>

MEMBER
struct cksum_params {
    typedef Word word_type;
    typedef Permute permute_type;
    typedef Hash hash_type;

    enum { cksum_size = CksumSize,
	   cksum_skip = CksumSkip,
	   compaction = Compaction,
    };
};


MEMBER
struct rabin_karp {
    typedef Word word_type;
    typedef Permute permute_type;
    typedef Hash hash_type;

    enum { cksum_size = CksumSize,
	   cksum_skip = CksumSkip, 
	   compaction = Compaction,
    };

    // (a^cksum_size-1 c_0) + (a^cksum_size-2 c_1) ...
    rabin_karp() {
	multiplier = good_word<Word>();
	powers = new Word[cksum_size];
	powers[cksum_size - 1] = 1;
	for (int i = cksum_size - 2; i >= 0; i--) {
	    powers[i] = powers[i + 1] * multiplier;
	}
	product = powers[0] * multiplier;
    }

    ~rabin_karp() {
	delete [] powers;
    }

    Word step(const uint8_t *ptr) {
	Word h = 0;
	for (int i = 0; i < cksum_size; i++) {
	    h += permute_type()(ptr[i]) * powers[i];
	}
	return h;
    }

    Word state0(const uint8_t *ptr) {
	incr_state = step(ptr);
	return incr_state;
    }

    Word incr(const uint8_t *ptr) {
	incr_state = multiplier * incr_state -
	    product * permute_type()(ptr[-1]) +
	    permute_type()(ptr[cksum_size - 1]);
	return incr_state;
    }

    Word *powers;
    Word  product;
    Word  multiplier;
    Word  incr_state;
};

MEMBER
struct adler32_cksum {
    typedef Word word_type;
    typedef Permute permute_type;
    typedef Hash hash_type;

    enum { cksum_size = CksumSize,
	   cksum_skip = CksumSkip, 
	   compaction = Compaction,
    };

    Word step(const uint8_t *ptr) {
	return xd3_lcksum (ptr, cksum_size);
    }

    Word state0(const uint8_t *ptr) {
	incr_state = step(ptr);
	return incr_state;
    }

    Word incr(const uint8_t *ptr) {
	incr_state = xd3_large_cksum_update (incr_state, ptr - 1, cksum_size);
	return incr_state;
    }

    Word  incr_state;
};

// TESTS

template <typename Word>
struct file_stats {
    typedef list<const uint8_t*> ptr_list;
    typedef Word word_type;
    typedef map<word_type, ptr_list> table_type;
    typedef typename table_type::iterator table_iterator;
    typedef typename ptr_list::iterator ptr_iterator;

    int cksum_size;
    int cksum_skip;
    int unique;
    int unique_values;
    int count;
    table_type table;

    file_stats(int size, int skip)
	: cksum_size(size),
	  cksum_skip(skip),
	  unique(0),
	  unique_values(0),
	  count(0) {
    }

    void reset() {
	unique = 0;
	unique_values = 0;
	count = 0;
	table.clear();
    }

    void update(const word_type &word, const uint8_t *ptr) {
	table_iterator t_i = table.find(word);

	count++;

	if (t_i == table.end()) {
	    table.insert(make_pair(word, ptr_list()));
	}

	ptr_list &pl = table[word];

	for (ptr_iterator p_i = pl.begin();
	     p_i != pl.end();
	     ++p_i) {
	    if (memcmp(*p_i, ptr, cksum_size) == 0) {
		return;
	    }
	}

	unique++;
	pl.push_back(ptr);
    }

    void freeze() {
	unique_values = table.size();
	table.clear();
    }
};

struct test_result_base;

static vector<test_result_base*> all_tests;

struct test_result_base {
    virtual ~test_result_base() {
    }
    virtual void reset() = 0;
    virtual void print() = 0;
    virtual void get(const uint8_t* buf, const int buf_size, int iters) = 0;
    virtual void stat() = 0;
    virtual int count() = 0;
    virtual int dups() = 0;
    virtual double uniqueness() = 0;
    virtual double fullness() = 0;
    virtual double collisions() = 0;
    virtual double coverage() = 0;
    virtual double compression() = 0;
    virtual double time() = 0;
    virtual double score() = 0;
    virtual void set_score(double min_dups_frac, double min_time) = 0;
    virtual double total_time() = 0;
    virtual int total_count() = 0;
    virtual int total_dups() = 0;
};

struct compare_h {
    bool operator()(test_result_base *a,
		    test_result_base *b) {
	return a->score() < b->score();
    }
};

MEMBER
struct test_result : public test_result_base {
    typedef Word word_type;
    typedef Permute permute_type;
    typedef Hash hash_type;

    enum { cksum_size = CksumSize,
	   cksum_skip = CksumSkip, 
	   compaction = Compaction,
    };

    const char *test_name;
    file_stats<Word> fstats;
    int test_size;
    int n_steps;
    int n_incrs;
    int s_bits;
    int s_mask;
    int t_entries;
    int h_bits;
    int h_buckets_full;
    double h_score;
    char *hash_table;
    long accum_millis;
    int accum_iters;

    // These are not reset
    double accum_time;
    int accum_count;
    int accum_dups;
    int accum_colls;
    int accum_size;

    test_result(const char *name)
	: test_name(name),
	  fstats(cksum_size, cksum_skip),
	  hash_table(NULL),
	  accum_millis(0),
	  accum_iters(0),
	  accum_time(0.0),
	  accum_count(0),
	  accum_dups(0),
	  accum_colls(0),
	  accum_size(0) {
	all_tests.push_back(this);
    }

    ~test_result() {
	reset();
    }

    void reset() {
	// size of file
	test_size = -1;

	// count
	n_steps = -1;
	n_incrs = -1;

	// four values used by new_table()/summarize_table()
	s_bits = -1;
	s_mask = -1;
	t_entries = -1;
	h_bits = -1;
	h_buckets_full = -1;

	accum_millis = 0;
	accum_iters = 0;

	fstats.reset();

	// temporary
	if (hash_table) {
	    delete(hash_table);
	    hash_table = NULL;
	}
    }

    int count() {
	if (cksum_skip == 1) {
	    return n_incrs;
	} else {
	    return n_steps;
	}
    }

    int dups() {
	return fstats.count - fstats.unique;
    }

    int colls() {
	return fstats.unique - fstats.unique_values;
    }

    double uniqueness() {
	return 1.0 - (double) dups() / count();
    }

    double fullness() {
	return (double) h_buckets_full / (1 << h_bits);
    }

    double collisions() {
	return (double) colls() / fstats.unique;
    }

    double coverage() {
	return (double) h_buckets_full / uniqueness() / count();
    }

    double compression() {
	return 1.0 - coverage();
    }

    double time() {
	return (double) accum_millis / accum_iters;
    }

    double score() {
	return h_score;
    }

    void set_score(double min_compression, double min_time) {
	h_score = (compression() - 0.99 * min_compression)
	        * (time() - 0.99 * min_time);
    }

    double total_time() {
	return accum_time;
    }

    int total_count() {
	return accum_count;
    }

    int total_dups() {
	return accum_dups;
    }

    int total_colls() {
	return accum_dups;
    }

    void stat() {
	accum_time += time();
	accum_count += count();
	accum_dups += dups();
	accum_colls += colls();
	accum_size += test_size;
    }

    void print() {
	if (fstats.count != count()) {
	    fprintf(stderr, "internal error: %d != %d\n", fstats.count, count());
	    abort();
	}
	printf("%s: (%u#%u) count %u uniq %0.2f%% full %u (%0.4f%% coll %0.4f%%) covers %0.2f%% w/ 2^%d @ %.4f MB/s %u iters\n",
	       test_name,
	       cksum_size,
	       cksum_skip,
	       count(),
	       100.0 * uniqueness(),
	       h_buckets_full,
	       100.0 * fullness(),
	       100.0 * collisions(),
	       100.0 * coverage(),
	       h_bits,
	       0.001 * accum_iters * test_size / accum_millis,
	       accum_iters);
    }

    int size_log2 (int slots)
    {
	int bits = bitsof<word_type>() - 1;
	int i;

	for (i = 3; i <= bits; i += 1) {
	    if (slots <= (1 << i)) {
		return i - compaction;
	    }
	}

	return bits;
    }

    void new_table(int entries) {
	t_entries = entries;
	h_bits = size_log2(entries);

	int n = 1 << h_bits;

	s_bits = bitsof<word_type>() - h_bits;
	s_mask = n - 1;

	hash_table = new char[n / 8];
	memset(hash_table, 0, n / 8);
    }

    int get_table_bit(int i) {
	return hash_table[i/8] & (1 << i%8);
    }

    int set_table_bit(int i) {
	return hash_table[i/8] |= (1 << i%8);
    }

    void summarize_table() {
	int n = 1 << h_bits;
	int f = 0;
	for (int i = 0; i < n; i++) {
	    if (get_table_bit(i)) {
		f++;
	    }
	}
	h_buckets_full = f;
    }

    void get(const uint8_t* buf, const int buf_size, int test_iters) {
	rabin_karp<SELF> test;
	//adler32_cksum<SELF> test;
	hash_type hash;
	const uint8_t *ptr;
	const uint8_t *end;
	int last_offset;
	int periods;
	int stop;

	test_size = buf_size;
	last_offset = buf_size - cksum_size;

	if (last_offset < 0) {
	    periods = 0;
	    n_steps = 0;
	    n_incrs = 0;
	    stop = -cksum_size;
	} else {
	    periods = last_offset / cksum_skip;
	    n_steps = periods + 1;
	    n_incrs = last_offset + 1;
	    stop = last_offset - (periods + 1) * cksum_skip;
	}

	// Compute file stats once.
	if (fstats.unique_values == 0) {
	    if (cksum_skip == 1) {
		for (int i = 0; i <= buf_size - cksum_size; i++) {
		    fstats.update(hash(test.step(buf + i), s_bits, s_mask), buf + i);
		}
	    } else {
		ptr = buf + last_offset;
		end = buf + stop;
		
		for (; ptr != end; ptr -= cksum_skip) {
		    fstats.update(hash(test.step(ptr), s_bits, s_mask), ptr);
		}
	    }
	    fstats.freeze();
	}

	long start_test = get_millisecs_now();

	if (cksum_skip != 1) {
	    new_table(n_steps);

	    for (int i = 0; i < test_iters; i++) {
		ptr = buf + last_offset;
		end = buf + stop;

		for (; ptr != end; ptr -= cksum_skip) {
		    set_table_bit(hash(test.step(ptr), s_bits, s_mask));
		}
	    }

	    summarize_table();
	}

	stop = buf_size - cksum_size + 1;
	if (stop < 0) {
	    stop = 0;
	}

	if (cksum_skip == 1) {

	    new_table(n_incrs);

	    for (int i = 0; i < test_iters; i++) {
		ptr = buf;
		end = buf + stop;

		if (ptr != end) {
		    set_table_bit(hash(test.state0(ptr++), s_bits, s_mask));
		}

		for (; ptr != end; ptr++) {
		    Word w = test.incr(ptr);
		    assert(w == test.step(ptr));
		    set_table_bit(hash(w, s_bits, s_mask));
		}
	    }

	    summarize_table();
	}

	accum_iters += test_iters;
	accum_millis += get_millisecs_now() - start_test;
    }
};

template <typename Word>
void print_array(const char *tname) {
    printf("static const %s hash_multiplier[64] = {\n", tname);
    Word p = 1;
    for (int i = 0; i < 64; i++) {
	printf("  %uU,\n", p);
	p *= good_word<Word>();
    }
    printf("};\n", tname);
}

int main(int argc, char** argv) {
  int i;
  uint8_t *buf = NULL;
  size_t buf_len = 0;
  int ret;

  if (argc <= 1) {
    fprintf(stderr, "usage: %s file ...\n", argv[0]);
    return 1;
  }

  //print_array<uint32_t>("uint32_t");

#define TEST(T,Z,S,P,H,C) test_result<T,Z,S,P,H<T>,C> \
      _ ## T ## _ ## Z ## _ ## S ## _ ## P ## _ ## H ## _ ## C \
      (#T "_" #Z "_" #S "_" #P "_" #H "_" #C)

#if 0

  TEST(uint32_t, 4, SKIP, plain, hhash, 0); /* x */ \
  TEST(uint32_t, 4, SKIP, plain, hhash, 1); /* x */ \
  TEST(uint32_t, 4, SKIP, plain, hhash, 2); /* x */ \
  TEST(uint32_t, 4, SKIP, plain, hhash, 3); /* x */ \

#endif

#define TESTS(SKIP) \
  TEST(uint32_t, 9, SKIP, plain, hhash, 0); /* x */ \
  TEST(uint32_t, 9, SKIP, plain, hhash, 1); /* x */ \
  TEST(uint32_t, 9, SKIP, plain, hhash, 2); /* x */ \
  TEST(uint32_t, 9, SKIP, plain, hhash, 3)
  
#define TESTS_ALL(SKIP) \
  TEST(uint32_t, 3, SKIP, plain, hhash, 0); \
  TEST(uint32_t, 3, SKIP, plain, hhash, 1); \
  TEST(uint32_t, 4, SKIP, plain, hhash, 0); /* x */ \
  TEST(uint32_t, 4, SKIP, plain, hhash, 1); /* x */ \
  TEST(uint32_t, 4, SKIP, plain, hhash, 2); /* x */ \
  TEST(uint32_t, 4, SKIP, plain, hhash, 3); /* x */ \
  TEST(uint32_t, 5, SKIP, plain, hhash, 0); \
  TEST(uint32_t, 5, SKIP, plain, hhash, 1); \
  TEST(uint32_t, 8, SKIP, plain, hhash, 0); \
  TEST(uint32_t, 8, SKIP, plain, hhash, 1); \
  TEST(uint32_t, 9, SKIP, plain, hhash, 0); /* x */ \
  TEST(uint32_t, 9, SKIP, plain, hhash, 1); /* x */ \
  TEST(uint32_t, 9, SKIP, plain, hhash, 2); /* x */ \
  TEST(uint32_t, 9, SKIP, plain, hhash, 3); /* x */ \
  TEST(uint32_t, 11, SKIP, plain, hhash, 0); /* x */ \
  TEST(uint32_t, 11, SKIP, plain, hhash, 1); /* x */ \
  TEST(uint32_t, 13, SKIP, plain, hhash, 0); \
  TEST(uint32_t, 13, SKIP, plain, hhash, 1); \
  TEST(uint32_t, 15, SKIP, plain, hhash, 0); /* x */ \
  TEST(uint32_t, 15, SKIP, plain, hhash, 1); /* x */ \
  TEST(uint32_t, 16, SKIP, plain, hhash, 0); /* x */ \
  TEST(uint32_t, 16, SKIP, plain, hhash, 1); /* x */ \
  TEST(uint32_t, 21, SKIP, plain, hhash, 0); \
  TEST(uint32_t, 21, SKIP, plain, hhash, 1); \
  TEST(uint32_t, 34, SKIP, plain, hhash, 0); \
  TEST(uint32_t, 34, SKIP, plain, hhash, 1); \
  TEST(uint32_t, 55, SKIP, plain, hhash, 0); \
  TEST(uint32_t, 55, SKIP, plain, hhash, 1)

  TESTS(1); // *
//   TESTS(2); // *
//   TESTS(3); // *
//   TESTS(5); // *
//   TESTS(8); // *
//   TESTS(9);
//   TESTS(11);
//   TESTS(13); // *
  TESTS(15);
//   TESTS(16);
//   TESTS(21); // *
//   TESTS(34); // *
//   TESTS(55); // *
//   TESTS(89); // *

  for (i = 1; i < argc; i++) {
    if ((ret = read_whole_file(argv[i],
			       & buf,
			       & buf_len))) {
      return 1;
    }

    fprintf(stderr, "file %s is %zu bytes\n",
	    argv[i], buf_len);

    double min_time = -1.0;
    double min_compression = 0.0;

    for (vector<test_result_base*>::iterator i = all_tests.begin();
	 i != all_tests.end(); ++i) {
	test_result_base *test = *i;
	test->reset();

	int iters = 100;
	long start_test = get_millisecs_now();

	do {
	    test->get(buf, buf_len, iters);
	    iters *= 3;
	    iters /= 2;
	} while (get_millisecs_now() - start_test < 2000);

	test->stat();

	if (min_time < 0.0) {
	    min_compression = test->compression();
	    min_time = test->time();
	}

	if (min_time > test->time()) {
	    min_time = test->time();
	}

	if (min_compression > test->compression()) {
	    min_compression = test->compression();
	}

	test->print();
    }

//     for (vector<test_result_base*>::iterator i = all_tests.begin();
// 	 i != all_tests.end(); ++i) {
// 	test_result_base *test = *i;
// 	test->set_score(min_compression, min_time);
//     }	

//     sort(all_tests.begin(), all_tests.end(), compare_h());
    
//     for (vector<test_result_base*>::iterator i = all_tests.begin();
// 	 i != all_tests.end(); ++i) {
// 	test_result_base *test = *i;
// 	test->print();
//     }	
    
    free(buf);
    buf = NULL;
  }

  return 0;      
}
