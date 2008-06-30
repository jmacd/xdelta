/* -*- Mode: C++ -*-  */
namespace regtest {

class Segment {
 public:
  Segment(uint32_t seed, size_t length)
    : seed(seed),
      length(length),
      seed_offset(0) { }

  uint32_t seed;  // Seed used for generating byte sequence
  size_t length;  // Length of this segment
  size_t seed_offset;  // Seed positions the sequence this many bytes
                       // before its beginning.
};

typedef map<xoff_t, Segment> SegmentMap;

class Block;
class BlockIterator;

class FileSpec {
 public:
  FileSpec(MTRandom *rand)
    : rand_(rand) {
  }

  // Generates a file with a known size
  void GenerateFixedSize(xoff_t size) {
    Reset();
    
    for (xoff_t p = 0; p < size; ) {
      xoff_t t = min(Constants::BLOCK_SIZE, size - p);
      table_.insert(make_pair(p, Segment(rand_->Rand32(), t)));
      p += t;
    }
  }

  // Generates a file with exponential-random distributed size
  void GenerateRandomSize(xoff_t mean) {
    GenerateFixedSize(rand_->ExpRand(mean));
  }

  // Returns the size of the file
  xoff_t Size() const {
    if (table_.empty()) {
      return 0;
    }
    SegmentMap::const_iterator i = --table_.end();
    return i->first + i->second.length;
  }

  // Returns the number of blocks
  xoff_t Blocks() const {
    if (table_.empty()) {
      return 0;
    }
    return ((Size() - 1) / Constants::BLOCK_SIZE) + 1;
  }

  // Returns the number of segments
  xoff_t Segments() const {
    return table_.size();
  }

  // Create a mutation according to "what".
  template <typename T>
  void ModifyTo(FileSpec *modify) const {
    modify->table_ = table_;
    T::Mutate(&modify->table_, rand_);
  }

  void Reset() {
    table_.clear();
  }

  typedef BlockIterator iterator;

 private:
  friend class BlockIterator;

  MTRandom *rand_;
  SegmentMap table_;
};

class Block {
public:
  Block()
    : data_(new uint8_t[Constants::BLOCK_SIZE]) { }
  ~Block() {
    delete [] data_;
  }
    
  size_t Size() const {
    return size_;
  }
  uint8_t operator[](size_t i) const {
    CHECK_LT(i, Constants::BLOCK_SIZE);
    return data_[i];
  }

private:
  friend class BlockIterator;

  size_t size_;
  uint8_t *data_;
};

class BlockIterator {
public:
  BlockIterator(const FileSpec& spec)
    : spec_(spec),
      blkno_(0) { }

  bool Done() const {
    return blkno_ >= spec_.Blocks();
  }

  void Next() {
    blkno_++;
  }

  void Get(Block *block) const;

  size_t BytesOnBlock() const {
    // Blocks() is unsigned, don't subtract
    if (blkno_ + 1 < spec_.Blocks()) {
      return Constants::BLOCK_SIZE;
    }
    return spec_.Size() % Constants::BLOCK_SIZE;
  }

private:
  const FileSpec& spec_;
  xoff_t blkno_;
};

inline void BlockIterator::Get(Block *block) const {
  xoff_t offset = blkno_ * Constants::BLOCK_SIZE;
  const SegmentMap &table = spec_.table_;
  size_t got = 0;
  block->size_ = BytesOnBlock();

  SegmentMap::const_iterator pos = table.upper_bound(offset);
  --pos;

  while (got < block->size_) {
    CHECK(pos != table.end());
    CHECK_GE(offset, pos->first);

    // The position of this segment may start before this block starts,
    // and then the position of the data may be offset from the seeding 
    // position.
    size_t seg_offset = offset - pos->first;
    xoff_t skip = seg_offset + pos->second.seed_offset;
    MTRandom gen(pos->second.seed);
    while (skip--) {
      gen.Rand32();
    }
    
    for (size_t i = seg_offset; i < pos->second.length; i++) {
      CHECK_LT(got, Constants::BLOCK_SIZE);
      block->data_[got++] = gen.Rand32();
    }

    offset += (pos->second.length - seg_offset);
    ++pos;
  }
}

}  // namespace regtest

