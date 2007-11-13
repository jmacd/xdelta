/* Copyright (C) 2007 Josh MacDonald */

extern "C" {
#include "test.h"
#include <assert.h>
}

#include <list>
#include <map>

using std::list;
using std::map;

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

struct no_permute {
    int operator()(const uint8_t &c) {
	return c;
    }
};

struct permute {
    int operator()(const uint8_t &c) {
	return __single_hash[c];
    }
};

template <typename Word>
struct both {
    Word operator()(const Word& t, const int &bits, const int &mask) {
	return (t >> bits) ^ (t & mask);
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

int
size_log2 (int slots)
{
  int bits = 31;
  int i;

  for (i = 3; i <= bits; i += 1)
    {
      if (slots <= (1 << i))
	{
	  bits = i;
	  break;
	}
    }

  return bits;
}

// CLASSES

#define SELF Word, CksumSize, CksumSkip, Permute, Hash
#define MEMBER template <typename Word, \
			 int CksumSize, \
			 int CksumSkip, \
			 typename Permute, \
			 typename Hash>

MEMBER
struct cksum_params {
    typedef Word word_type;
    typedef Permute permute_type;
    typedef Hash hash_type;

    enum { cksum_size = CksumSize,
	   cksum_skip = CksumSkip, 
    };
};


MEMBER
struct rabin_karp {
    typedef Word word_type;
    typedef Permute permute_type;
    typedef Hash hash_type;

    enum { cksum_size = CksumSize,
	   cksum_skip = CksumSkip, 
    };

    // In this code, we use (a^cksum_size-1 c_0) + (a^cksum_size-2 c_1) ...
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
	table.clear();
	unique_values = table.size();
    }
};

struct test_result_base;

static list<test_result_base*> all_tests;

struct test_result_base {
    virtual ~test_result_base() {
    }
    virtual void reset() = 0;
    virtual void print() = 0;
    virtual void get(const uint8_t* buf, const int buf_size, int iters) = 0;
};

MEMBER
struct test_result : public test_result_base {
    typedef Word word_type;
    typedef Permute permute_type;
    typedef Hash hash_type;

    enum { cksum_size = CksumSize,
	   cksum_skip = CksumSkip, 
    };

    const char *test_name;
    file_stats<Word> fstats;
    int test_size;
    int test_iters;
    int n_steps;
    int n_incrs;
    int s_bits;
    int s_mask;
    int t_entries;
    int h_bits;
    double h_fill;
    double l_fill;
    char *hash_table;
    long start_test, end_test;

    test_result(const char *name)
	: test_name(name),
	  fstats(cksum_size, cksum_skip),
	  hash_table(NULL) {
	all_tests.push_back(this);
    }

    ~test_result() {
	reset();
    }

    void reset() {
	test_size = -1;
	test_iters = -1;
	n_steps = -1;
	n_incrs = -1;
	s_bits = -1;
	s_mask = -1;
	t_entries = -1;
	h_bits = -1;
	h_fill = 0.0;
	l_fill = 0.0;
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

    void print() {

	printf("%s: (%u#%u) count %u dups %0.2f%% coll %0.2f%% fill %0.2f%% heff %0.2f%% %.4f MB/s\n",
	       test_name,
	       cksum_size,
	       cksum_skip,
	       count(),
	       100.0 * (fstats.count - fstats.unique) / fstats.count,
	       100.0 * (fstats.unique - fstats.unique_values) / fstats.unique,
	       100.0 * h_fill,
	       100.0 * l_fill,
	       0.001 * test_iters * test_size / (end_test - start_test));
    }

    void new_table(int entries) {
	t_entries = entries;
	h_bits = size_log2(entries);
	s_bits = bitsof<word_type>() - h_bits;
	s_mask = (1 << h_bits) - 1;

	int n = 1 << h_bits;
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
	h_fill = (double) f / (double) t_entries;
	l_fill = (double) f / (double) fstats.unique;
    }

    void get(const uint8_t* buf, const int buf_size, int iters) {
	rabin_karp<SELF> test;
	hash_type hash;
	const uint8_t *ptr;
	const uint8_t *end;
	int last_offset;
	int periods;
	int stop;

	test_size = buf_size;
	test_iters = iters;
	last_offset = buf_size - cksum_size;

	if (last_offset < 0) {
	    periods = 0;
	    n_steps = 0;
	    n_incrs = 0;
	    stop = -cksum_size;
	} else {
	    periods = last_offset / cksum_skip;
	    n_steps = periods + 1;
	    n_incrs = last_offset;
	    stop = last_offset - (periods + 1) * cksum_skip;
	}

	// Compute file stats
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

	start_test = get_millisecs_now();

	if (cksum_skip != 1) {
	    new_table(n_steps);

	    for (int i = 0; i < iters; i++) {
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

	    for (int i = 0; i < iters; i++) {
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

	end_test = get_millisecs_now();
    }
};

int main(int argc, char** argv) {
  int i;
  uint8_t *buf = NULL;
  usize_t buf_len = 0;
  int ret;

  if (argc <= 1) {
    fprintf(stderr, "usage: %s file ...\n", argv[0]);
    return 1;
  }

#define TEST(T,Z,S,P,H) test_result<T,Z,S,P,H<T> > \
      _ ## T ## Z ## S ## P ## H (#T "_" #Z "_" #S "_" #P "_" #H)

  TEST(uint32_t, 4, 1, no_permute, both);
  TEST(uint32_t, 4, 2, no_permute, both);
  TEST(uint32_t, 4, 3, no_permute, both);
  TEST(uint32_t, 4, 4, no_permute, both);

  TEST(uint32_t, 7, 15, no_permute, both);
  TEST(uint32_t, 7, 55, no_permute, both);

  TEST(uint32_t, 9, 1, no_permute, both);
  TEST(uint32_t, 9, 2, no_permute, both);
  TEST(uint32_t, 9, 3, no_permute, both);
  TEST(uint32_t, 9, 4, no_permute, both);
  TEST(uint32_t, 9, 5, no_permute, both);
  TEST(uint32_t, 9, 6, no_permute, both);
  TEST(uint32_t, 9, 7, no_permute, both);
  TEST(uint32_t, 9, 8, no_permute, both);
  TEST(uint32_t, 9, 15, no_permute, both);
  TEST(uint32_t, 9, 26, no_permute, both);
  TEST(uint32_t, 9, 55, no_permute, both);

  TEST(uint32_t, 10, 14, no_permute, both);
  TEST(uint32_t, 10, 25, no_permute, both);
  TEST(uint32_t, 10, 54, no_permute, both);

  TEST(uint32_t, 11, 13, no_permute, both);
  TEST(uint32_t, 11, 24, no_permute, both);
  TEST(uint32_t, 11, 53, no_permute, both);

  TEST(uint32_t, 16, 16, no_permute, both);
  TEST(uint32_t, 16, 31, no_permute, both);
  TEST(uint32_t, 16, 32, no_permute, both);
  TEST(uint32_t, 16, 48, no_permute, both);

  for (i = 1; i < argc; i++) {
    if ((ret = read_whole_file(argv[i],
			       & buf,
			       & buf_len))) {
      return 1;
    }

    int target = 100 << 20;
    int iters = (target / buf_len) + 1;

    fprintf(stderr, "file %s is %u bytes %u iters\n",
	    argv[i], buf_len, iters);

    for (list<test_result_base*>::iterator i = all_tests.begin();
	 i != all_tests.end(); ++i) {
	(*i)->reset();
	(*i)->get(buf, buf_len, iters);
	(*i)->print();
    }

    free(buf);
    buf = NULL;
  }

  return 0;      
}
