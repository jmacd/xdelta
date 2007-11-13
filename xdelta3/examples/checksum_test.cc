/* Copyright (C) 2007 Josh MacDonald */

extern "C" {
#include "test.h"
#include <assert.h>
}

#include <list>
#include <map>

using std::list;
using std::map;

// Need gcc4
// template <typename T, int TestCklen>
// struct cksum_params {
//   typedef T cksum_type;
//   enum { test_cklen = TestCklen };
// };


// MLCG parameters
uint32_t good_32bit_values[] = {
//    741103597U, 887987685U,
    1597334677U,
};

// a, a*
uint64_t good_64bit_values[] = {
//    1181783497276652981ULL, 4292484099903637661ULL,
    7664345821815920749ULL,
};

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

struct noperm {
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

template <typename Word, int CksumSize, int CksumSkip, typename Permute>
struct rabin_karp {
    typedef Word word_type;
    typedef Permute permute_type;

    enum { cksum_size = CksumSize,
	   cksum_skip = CksumSkip, 
    };

    rabin_karp() {
	multiplier = good_word<Word>();
	powers = new Word[cksum_size];
	powers[cksum_size - 1] = 1;
	for (int i = cksum_size - 2; i >= 0; i--) {
	    powers[i] = powers[i + 1] * multiplier;
	}
	product = powers[0] * multiplier;
    }

    Word step(const uint8_t *ptr) {
	Word h = 0;
	for (int i = 0; i < cksum_size; i++) {
	    h += permute_type()(ptr[i]) * powers[i];
	}
	return h;
    }

    Word state0(const uint8_t *ptr) {
	state = step(ptr);
	return state;
    }

    Word incr(const uint8_t *ptr) {
	state = state -
	    product * permute_type()(ptr[-1]) +
	    permute_type()(ptr[cksum_size - 1]);
	return state;
    }

    Word *powers;
    Word  product;
    Word  multiplier;
    Word  state;
};

struct test_result_base;

static list<test_result_base*> all_tests;

struct test_result_base {
    virtual ~test_result_base() {
    }
    virtual void print() = 0;
    virtual void get(const uint8_t* buf, const int buf_size) = 0;
};

template<typename T>
struct test_result : public test_result_base {
    int n_steps;
    int n_incrs;
    int s_bits;
    int s_mask;
    int h_bits;
    int t_entries;
    double step_fill;
    double incr_fill;    
    long start_step, end_step;
    long start_incr, end_incr;

    test_result() {
	all_tests.push_back(this);
    }

    void print() {
	fprintf(stderr,
		"cksum size %u: skip %u: %u steps: %u incrs: "
		"s_fill %0.2f%% i_fill %0.2f%%\n",
		T::cksum_size,
		T::cksum_skip,
		n_steps,
		n_incrs,
		100.0 * step_fill,
		100.0 * incr_fill);
    }

    int* new_table(int entries) {
	t_entries = entries;
	h_bits = size_log2(entries);
	s_bits = bitsof<typename T::word_type>() - h_bits;
	s_mask = (1 << h_bits) - 1;

	int n = 1 << h_bits;
	int *t = new int[n];
	memset(t, 0, sizeof(int) * n);
	return t;
    }

    double summarize_table(int* table) {
	int n = 1 << h_bits;
	int f = 0;
	for (int i = 0; i < n; i++) {
	    if (table[i] != 0) {
		f++;
	    }
	}
	delete [] table;
	return (double) f / (double) t_entries;
    }

    void get(const uint8_t* buf, const int buf_size) {
	const uint8_t *ptr;
	const uint8_t *end;
	int last_offset;
	int periods;
	int stop;
	int *hash_table;

	last_offset = buf_size - T::cksum_size;

	if (last_offset < 0) {
	    periods = 0;
	    n_steps = 0;
	    n_incrs = 0;
	    stop = -T::cksum_size;
	} else {
	    periods = last_offset / T::cksum_skip;
	    n_steps = periods;
	    n_incrs = last_offset;
	    stop = last_offset - (periods + 1) * T::cksum_skip;
	}

	hash_table = new_table(n_steps);

	start_step = get_millisecs_now();

	ptr = buf + last_offset;
	end = buf + stop;

	T t;
	typename T::word_type w;

	for (; ptr != end; ptr -= T::cksum_skip) {
	    w = t.step(ptr);
	    ++hash_table[(w >> s_bits) ^ (w & s_mask)];
	}

	end_step = get_millisecs_now();

	step_fill = summarize_table(hash_table);
	hash_table = new_table(n_incrs);

	stop = buf_size - T::cksum_size + 1;
	if (stop < 0) {
	    stop = 0;
	}

	start_incr = end_step;

	ptr = buf;
	end = buf + stop;
	if (ptr != end) {
	    w = t.state0(ptr++);
	    ++hash_table[(w >> s_bits) ^ (w & s_mask)];
	}
	for (; ptr != end; ptr++) {
	    w = t.incr(ptr);
	    ++hash_table[(w >> s_bits) ^ (w & s_mask)];
	}

	end_incr = get_millisecs_now();

	incr_fill = summarize_table(hash_table);
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

  test_result<rabin_karp<uint32_t, 4, 1, noperm> > small_1_cksum;
  test_result<rabin_karp<uint32_t, 4, 4, noperm> > small_4_cksum;
  test_result<rabin_karp<uint32_t, 9, 1, noperm> > large_1_cksum;  
  test_result<rabin_karp<uint32_t, 9, 2, noperm> > large_2_cksum;  
  test_result<rabin_karp<uint32_t, 9, 3, noperm> > large_3_cksum;
  test_result<rabin_karp<uint32_t, 9, 5, noperm> > large_5_cksum;  
  test_result<rabin_karp<uint32_t, 9, 6, noperm> > large_6_cksum;  
  test_result<rabin_karp<uint32_t, 9, 7, noperm> > large_7_cksum;  
  test_result<rabin_karp<uint32_t, 9, 8, noperm> > large_8_cksum;     
  test_result<rabin_karp<uint32_t, 9, 15, noperm> > large_15_cksum;  
  test_result<rabin_karp<uint32_t, 9, 26, noperm> > large_26_cksum; 
  test_result<rabin_karp<uint32_t, 9, 55, noperm> > large_55_cksum; 

  test_result<rabin_karp<uint32_t, 4, 1, permute> > small_1_cksum_p;
  test_result<rabin_karp<uint32_t, 4, 4, permute> > small_4_cksum_p;
  test_result<rabin_karp<uint32_t, 9, 1, permute> > large_1_cksum_p;
  test_result<rabin_karp<uint32_t, 9, 2, permute> > large_2_cksum_p;
  test_result<rabin_karp<uint32_t, 9, 3, permute> > large_3_cksum_p;
  test_result<rabin_karp<uint32_t, 9, 5, permute> > large_5_cksum_p;
  test_result<rabin_karp<uint32_t, 9, 6, permute> > large_6_cksum_p;
  test_result<rabin_karp<uint32_t, 9, 7, permute> > large_7_cksum_p;
  test_result<rabin_karp<uint32_t, 9, 8, permute> > large_8_cksum_p;
  test_result<rabin_karp<uint32_t, 9, 15, permute> > large_15_cksum_p;
  test_result<rabin_karp<uint32_t, 9, 26, permute> > large_26_cksum_p;
  test_result<rabin_karp<uint32_t, 9, 55, permute> > large_55_cksum_p;

  test_result<rabin_karp<uint64_t, 4, 1, noperm> > small_1_cksum_64;
  test_result<rabin_karp<uint64_t, 4, 4, noperm> > small_4_cksum_64;
  test_result<rabin_karp<uint64_t, 9, 1, noperm> > large_1_cksum_64;
  test_result<rabin_karp<uint64_t, 9, 2, noperm> > large_2_cksum_64;
  test_result<rabin_karp<uint64_t, 9, 3, noperm> > large_3_cksum_64;
  test_result<rabin_karp<uint64_t, 9, 5, noperm> > large_5_cksum_64;
  test_result<rabin_karp<uint64_t, 9, 6, noperm> > large_6_cksum_64;
  test_result<rabin_karp<uint64_t, 9, 7, noperm> > large_7_cksum_64;
  test_result<rabin_karp<uint64_t, 9, 8, noperm> > large_8_cksum_64;
  test_result<rabin_karp<uint64_t, 9, 15, noperm> > large_15_cksum_64;
  test_result<rabin_karp<uint64_t, 9, 26, noperm> > large_26_cksum_64;
  test_result<rabin_karp<uint64_t, 9, 55, noperm> > large_55_cksum_64;

  test_result<rabin_karp<uint64_t, 4, 1, permute> > small_1_cksum_p_64;
  test_result<rabin_karp<uint64_t, 4, 4, permute> > small_4_cksum_p_64;
  test_result<rabin_karp<uint64_t, 9, 1, permute> > large_1_cksum_p_64;
  test_result<rabin_karp<uint64_t, 9, 2, permute> > large_2_cksum_p_64;
  test_result<rabin_karp<uint64_t, 9, 3, permute> > large_3_cksum_p_64;
  test_result<rabin_karp<uint64_t, 9, 5, permute> > large_5_cksum_p_64;
  test_result<rabin_karp<uint64_t, 9, 6, permute> > large_6_cksum_p_64;
  test_result<rabin_karp<uint64_t, 9, 7, permute> > large_7_cksum_p_64;
  test_result<rabin_karp<uint64_t, 9, 8, permute> > large_8_cksum_p_64;
  test_result<rabin_karp<uint64_t, 9, 15, permute> > large_15_cksum_p_64;
  test_result<rabin_karp<uint64_t, 9, 26, permute> > large_26_cksum_p_64;
  test_result<rabin_karp<uint64_t, 9, 55, permute> > large_55_cksum_p_64;

  for (i = 1; i < argc; i++) {
    if ((ret = read_whole_file(argv[i],
			       & buf,
			       & buf_len))) {
      return 1;
    }

    fprintf(stderr, "file %s is %u bytes\n", argv[i], buf_len);

    for (list<test_result_base*>::iterator i = all_tests.begin();
	 i != all_tests.end(); ++i) {
	(*i)->get(buf, buf_len);
	(*i)->print();
    }

    free(buf);
    buf = NULL;
  }

  return 0;      
}
