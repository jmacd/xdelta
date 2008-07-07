/* -*- Mode: C++ -*-  */
/* This is public-domain Mersenne Twister code,
 * attributed to Michael Brundage.  Thanks!
 * http://www.qbrundage.com/michaelb/pubs/essays/random_number_generation.html
 */
#include <math.h>

namespace regtest {

class MTRandom {
 public:
  static const uint32_t TEST_SEED1 = 0x1975;

  static const int MT_LEN = 624;
  static const int MT_IA = 397;
  static const int MT_IB = (MT_LEN - MT_IA);
  static const uint32_t UPPER_MASK = 0x80000000;
  static const uint32_t LOWER_MASK = 0x7FFFFFFF;
  static const uint32_t MATRIX_A = 0x9908B0DF;

  MTRandom() {
    Init(TEST_SEED1);
  }

  MTRandom(uint32_t seed) {
    Init(seed);
  }

  uint32_t TWIST(int i, int j) {
    return (mt_buffer_[i] & UPPER_MASK) | (mt_buffer_[j] & LOWER_MASK);
  }

  uint32_t MAGIC(uint32_t s) {
    return (s & 1) * MATRIX_A;
  }

  uint32_t Rand32 () {
    int idx = mt_index_;
    uint32_t s;
    int i;
	
    if (idx == MT_LEN * sizeof(uint32_t)) {
      idx = 0;
      i = 0;
      for (; i < MT_IB; i++) {
	s = TWIST(i, i+1);
	mt_buffer_[i] = mt_buffer_[i + MT_IA] ^ (s >> 1) ^ MAGIC(s);
      }
      for (; i < MT_LEN-1; i++) {
	s = TWIST(i, i+1);
	mt_buffer_[i] = mt_buffer_[i - MT_IB] ^ (s >> 1) ^ MAGIC(s);
      }
        
      s = TWIST(MT_LEN-1, 0);
      mt_buffer_[MT_LEN-1] = mt_buffer_[MT_IA-1] ^ (s >> 1) ^ MAGIC(s);
    }
    mt_index_ = idx + sizeof(uint32_t);

    // Original code had an unaligned access, make it portable.
    //   return *(uint32_t *)((unsigned char *)b + idx);
    uint32_t r;
    memcpy(&r, ((unsigned char *)mt_buffer_ + idx), sizeof(uint32_t));
    return r;
  }

  uint32_t ExpRand32(uint32_t mean) {
    double mean_d = mean;
    double erand  = log (1.0 / (Rand32() / (double)UINT32_MAX));
    uint32_t x = (uint32_t) (mean_d * erand + 0.5);
    return x;
  }

  uint64_t Rand64() {
    return ((uint64_t)Rand32() << 32) | Rand32();
  }

  uint64_t ExpRand64(uint64_t mean) {
    double mean_d = mean;
    double erand  = log (1.0 / (Rand64() / (double)UINT32_MAX));
    uint64_t x = (uint64_t) (mean_d * erand + 0.5);
    return x;
  }

  template <typename T>
  T Rand() {
    switch (sizeof(T)) {
    case sizeof(uint32_t):
      return Rand32();
    case sizeof(uint64_t):
      return Rand64();
    default:
      cerr << "Invalid sizeof T" << endl;
      abort();
    }
  }

  template <typename T>
  T ExpRand(T mean) {
    switch (sizeof(T)) {
    case sizeof(uint32_t):
      return ExpRand32(mean);
    case sizeof(uint64_t):
      return ExpRand64(mean);
    default:
      cerr << "Invalid sizeof T" << endl;
      abort();
    }
  }

 private:
  void Init(uint32_t seed) {
    int i;
    srand (seed);
    for (i = 0; i < MT_LEN; i++)
      {
	mt_buffer_[i] = rand ();
      }
    mt_index_ = 0;
  }

  int mt_index_;
  uint32_t mt_buffer_[MT_LEN];
};

class MTRandom8 {
public:
  MTRandom8(MTRandom *rand)
    : rand_(rand) {
  }

  uint8_t Rand8() {
    uint32_t r = rand_->Rand32();

    return (r & 0xff) ^ (r >> 8) ^ (r >> 16) ^ (r >> 24);
  }

private:
  MTRandom *rand_;
};

}  // namespace regtest
