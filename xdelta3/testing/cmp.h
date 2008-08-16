/* -*- Mode: C++ -*-  */
namespace regtest {

inline size_t CmpDifferentBlockBytes(const Block &a, const Block &b) {
  size_t total = 0;
  size_t i = 0; 
  size_t m = min(a.Size(), b.Size());

  for (; i < m; i++) {
    if (a[i] != b[i]) {
      total++;
    }
  }

  total += a.Size() - i;
  total += b.Size() - i;

  return total;
}

inline xoff_t CmpDifferentBytes(const FileSpec &a, const FileSpec &b) {
  Block block_a, block_b;
  xoff_t total = 0;
  FileSpec::iterator a_i(a), b_i(b);

  for (; !a_i.Done() && !b_i.Done(); a_i.Next(), b_i.Next()) {

    a_i.Get(&block_a);
    b_i.Get(&block_b);

    total += CmpDifferentBlockBytes(block_a, block_b);
  }

  for (; !a_i.Done(); a_i.Next()) {
    total += a_i.BytesOnBlock();
  }
  for (; !b_i.Done(); b_i.Next()) {
    total += b_i.BytesOnBlock();
  }

  return total;
}

inline bool ExtFile::EqualsSpec(const FileSpec &spec) const {
  main_file t;
  main_file_init(&t);
  CHECK_EQ(0, main_file_open(&t, Name(), XO_READ));

  Block tblock;
  Block sblock;
  for (BlockIterator iter(spec); !iter.Done(); iter.Next()) {
    iter.Get(&sblock);
    tblock.SetSize(sblock.Size());
    usize_t tread;
    CHECK_EQ(0, main_file_read(&t, tblock.Data(), tblock.Size(), &tread, "read failed"));
    CHECK_EQ(0, CmpDifferentBlockBytes(tblock, sblock));
  }
  
  CHECK_EQ(0, main_file_close(&t));
  main_file_cleanup(&t);
  return true;
}

}  // namespace regtest
