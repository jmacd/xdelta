/* -*- Mode: C++ -*-  */
namespace regtest {

class Modify1stByte {
 public:
  static void Mutate(SegmentMap *table, MTRandom *rand) {
    CHECK(!table->empty());

    SegmentMap::iterator i1 = table->begin();
    CHECK_EQ(0, i1->first);
    
    if (i1->second.length == 1) {
      i1->second.seed = rand->Rand32();
    } else {
      i1->second.seed_offset += 1;
      i1->second.length -= 1;
      table->insert(make_pair(1, i1->second));
      i1->second = Segment(rand->Rand32(), 1);
    }
  }
};

}  // namespace regtest
