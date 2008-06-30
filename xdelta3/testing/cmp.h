/* -*- Mode: C++ -*-  */
namespace regtest {

inline xoff_t CmpDifferentBytes(const FileSpec &a, const FileSpec &b) {
  Block block_a, block_b;
  xoff_t total = 0;
  FileSpec::iterator a_i(a), b_i(b);

  for (; !a_i.Done() && !b_i.Done(); a_i.Next(), b_i.Next()) {

    a_i.Get(&block_a);
    b_i.Get(&block_b);

    size_t i = 0; 
    size_t m = min(block_a.Size(), block_b.Size());

    for (; i < m; i++) {
      if (block_a[i] != block_b[i]) {
	total++;
      }
    }

    total += block_a.Size() - i;
    total += block_b.Size() - i;
  }

  for (; !a_i.Done(); a_i.Next()) {
    total += a_i.BytesOnBlock();
  }
  for (; !b_i.Done(); b_i.Next()) {
    total += b_i.BytesOnBlock();
  }

  return total;
}

}  // namespace regtest
