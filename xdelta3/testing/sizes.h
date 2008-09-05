// -*- Mode: C++ -*-
namespace regtest {

template <typename T, typename U>
class SizeIterator {
 public:
  SizeIterator(MTRandom *rand, size_t howmany)
    : rand_(rand),
      count_(0),
      fixed_(U::sizes),
      fixed_size_(SIZEOF_ARRAY(U::sizes)),
      howmany_(howmany) { }

  T Get() {
    if (count_ < fixed_size_) {
      return fixed_[count_];
    }
    return rand_->Rand<T>() % U::max_value;
  }

  bool Done() {
    return count_ >= fixed_size_ && count_ >= howmany_;
  }

  void Next() {
    count_++;
  }

 private:
  MTRandom *rand_;
  size_t count_;
  T* fixed_;
  size_t fixed_size_;
  size_t howmany_;
};

class SmallSizes {
public:
  static size_t sizes[];
  static size_t max_value;
};

size_t SmallSizes::sizes[] = {
  0, 1, Constants::BLOCK_SIZE / 4, 3333, 
  Constants::BLOCK_SIZE - (Constants::BLOCK_SIZE / 3),
  Constants::BLOCK_SIZE,
  Constants::BLOCK_SIZE + (Constants::BLOCK_SIZE / 3),
  2 * Constants::BLOCK_SIZE - (Constants::BLOCK_SIZE / 3),
  2 * Constants::BLOCK_SIZE,
  2 * Constants::BLOCK_SIZE + (Constants::BLOCK_SIZE / 3),
};

size_t SmallSizes::max_value = Constants::BLOCK_SIZE * 3;

class LargeSizes {
public:
  static size_t sizes[];
  static size_t max_value;
};

size_t LargeSizes::sizes[] = {
  1 << 20,
  1 << 18,
  1 << 16,
};

size_t LargeSizes::max_value = 1<<20;

}  // namespace regtest
