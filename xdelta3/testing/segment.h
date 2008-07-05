// -*- Mode: C++ -*-

namespace regtest {

class Segment {
 public:
  Segment(uint32_t seed, size_t length)
    : seed(seed),
      length(length),
      seed_offset(0) { 
    CHECK_GT(length, 0);
  }

  Segment(uint32_t seed, size_t length, 
	  size_t seed_offset)
    : seed(seed),
      length(length),
      seed_offset(seed_offset) {
    CHECK_GT(length, 0);
  }

  uint32_t seed;  // Seed used for generating byte sequence
  size_t length;  // Length of this segment
  size_t seed_offset;  // Seed positions the sequence this many bytes
                       // before its beginning.
};

typedef map<xoff_t, Segment> SegmentMap;

}  // namespace regtest
